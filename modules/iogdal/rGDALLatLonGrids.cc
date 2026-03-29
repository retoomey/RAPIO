#include "rGDALLatLonGrids.h"
#include "rOS.h"
#include "rLatLonGrid.h"

#include <gdal_priv.h>
#include <ogr_spatialref.h> // Required to set the projection
#include <cpl_conv.h> // for CPLMalloc()
#include <cmath>

using namespace rapio;

void
GDALLatLonGrids::introduceSelf(IODataType * owner)
{
  std::shared_ptr<IOSpecializer> io = std::make_shared<GDALLatLonGrids>();

  // Register the DataTypes we handle
  owner->introduce("LatLonGrid", io);
}

std::shared_ptr<DataType>
GDALLatLonGrids::read(
  std::map<std::string, std::string>& keys,
  std::shared_ptr<DataType>         dt)
{
  // The parent IOGDAL factory must place the actual file path here
  std::string filepath = keys["FilePath"];
  
  if (!filepath.empty()) {
    return readGDALGrid(filepath);
  } else {
    fLogSevere("GDALLatLonGrids: Invalid or missing FilePath in keys. Cannot read.");
  }
  return nullptr;
}

std::shared_ptr<DataType>
GDALLatLonGrids::readGDALGrid(const std::string& filepath)
{
  // 1. Ensure GDAL is registered (safe to call multiple times)
  GDALAllRegister();

  // 2. Open the Dataset
  GDALDataset* poDataset = static_cast<GDALDataset*>(GDALOpen(filepath.c_str(), GA_ReadOnly));
  if (poDataset == nullptr) {
    fLogSevere("GDAL reader: Failed to open dataset at {}", filepath);
    return nullptr;
  }

  // 3. Extract Spatial Metadata
  double adfGeoTransform[6];
  if (poDataset->GetGeoTransform(adfGeoTransform) != CE_None) {
    fLogSevere("GDAL reader: Dataset lacks spatial georeferencing (GeoTransform).");
    GDALClose(poDataset);
    return nullptr;
  }

  const float lonNWDegs      = static_cast<float>(adfGeoTransform[0]);
  const float lonSpacingDegs = static_cast<float>(adfGeoTransform[1]);
  const float latNWDegs      = static_cast<float>(adfGeoTransform[3]);
  const float latSpacingDegs = std::abs(static_cast<float>(adfGeoTransform[5])); // Enforce positive spacing

  const int num_x = poDataset->GetRasterXSize();
  const int num_y = poDataset->GetRasterYSize();

  fLogInfo("GDAL reader: Loaded {}x{} grid. NW: ({},{}), Res: {}x{}", 
           num_x, num_y, latNWDegs, lonNWDegs, latSpacingDegs, lonSpacingDegs);

  // 4. Create the rapio LatLonGrid
  LLH location(latNWDegs, lonNWDegs, 0.0);

  // ------------------------------------
  // Time attempt.  Might be able to move this up
  // 'higher' as we implement more gdal stuff
  //
  Time dataTime = Time::CurrentTime();

  // Check standard TIFF datetime tag first
  const char* timeStr = poDataset->GetMetadataItem("TIFFTAG_DATETIME");
  
  // If not found, you can check other common metadata keys depending on the formats you expect
  if (!timeStr) {
      timeStr = poDataset->GetMetadataItem("TIME"); 
  }

  if (timeStr) {
    try {
      // TIFF metadata format is standard: "YYYY:MM:DD HH:MM:SS"
      dataTime = Time(timeStr, "%Y:%m:%d %H:%M:%S");
    } catch (...) {
      fLogSevere("GDAL reader: Failed to parse time string: {}", timeStr);
    }
  } else {
    // Fallback: If no metadata exists, use the file modification time
    if (!OS::getFileModificationTime(filepath, dataTime)) {
      fLogInfo("GDAL reader: No time metadata found, defaulting to Epoch.");
    }
  }
  // ------------------------------------


  // FIXME: Should we pass down the name, etc.? How to get units, etc.
  // from the data.
  auto latLonGridSP = LatLonGrid::Create(
    "DEM",  // Make match our older files      
    "Meters",                   
    location,
    dataTime,
    latSpacingDegs,
    lonSpacingDegs,
    num_y, 
    num_x  
  );

  LatLonGrid& grid = *latLonGridSP;
  grid.setReadFactory("gdal");

  // 5. Extract the Raster Band and NoData value
  GDALRasterBand* poBand = poDataset->GetRasterBand(1);
  int bGotNoData;
  double gdalNoDataValue = poBand->GetNoDataValue(&bGotNoData);

  // 6. Access rapio's primary data memory
  auto array = grid.getFloat2D(Constants::PrimaryDataName);
  auto& data = array->ref();

#if 0
  // 7. Read Data Row-by-Row
  std::vector<float> scanline(num_x);
  for (int j = 0; j < num_y; ++j) {
    CPLErr err = poBand->RasterIO(GF_Read, 
                                  0, j, num_x, 1,            // Source Window
                                  scanline.data(), num_x, 1, // Destination Buffer
                                  GDT_Float32,               // Target Data Type
                                  0, 0);

    if (err != CE_None) {
      fLogSevere("GDAL reader: RasterIO read error at row {}", j);
      break;
    }

    // Transfer scanline to jagged array, handling NoData substitution
    for (int i = 0; i < num_x; ++i) {
      if (bGotNoData && scanline[i] == static_cast<float>(gdalNoDataValue)) {
        data[j][i] = Constants::MissingData; 
      } else {
        data[j][i] = scanline[i];
      }
    }
  }
#endif
  // 7. Read Data Row-by-Row Directly into rapio's memory
  for (int j = 0; j < num_y; ++j) {
    CPLErr err = poBand->RasterIO(GF_Read, 
                                  0, j, num_x, 1,             // Source Window (X, Y, Width, Height)
                                  &data[j][0], num_x, 1,      // DESTINATION: raw pointer to rapio's row start
                                  GDT_Float32,                // Target Data Type
                                  0, 0);

    if (err != CE_None) {
      fLogSevere("GDAL reader: RasterIO read error at row {}", j);
      break;
    }

    // In-place mapping of GDAL NoData to rapio's MissingData constant
    if (bGotNoData) {
      float gdalMissing = static_cast<float>(gdalNoDataValue);
      for (int i = 0; i < num_x; ++i) {
        if (data[j][i] == gdalMissing) {
          data[j][i] = Constants::MissingData; 
        }
      }
    }
  }

  GDALClose(poDataset);
  fLogInfo(">> Finished reading GDAL LatLonGrid");

  return latLonGridSP;
}

bool
GDALLatLonGrids::write(
  std::shared_ptr<DataType>         dt,
  std::map<std::string, std::string>& keys)
{
  // 1. Validate the DataType is a LatLonGrid
  auto latLonGrid = std::dynamic_pointer_cast<LatLonGrid>(dt);
  if (latLonGrid == nullptr) {
    fLogSevere("GdalLatLonGrids: Writer expected a LatLonGrid, but received a different DataType.");
    return false;
  }

  // 2. Extract File Path and Driver Key
  std::string filepath = keys["FilePath"];
  if (filepath.empty()) {
    fLogSevere("GdalLatLonGrids: No FilePath provided in keys.");
    return false;
  }

  // Use "GTiff" as the default driver, but allow the user/config to override
  std::string driverName = keys.count("GDAL_DRIVER") ? keys["GDAL_DRIVER"] : "GTiff";

  // 3. Initialize GDAL and Fetch the Driver
  GDALAllRegister();
  GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName(driverName.c_str());
  if (poDriver == nullptr) {
    fLogSevere("GdalLatLonGrids: GDAL Driver '{}' is not available or not recognized.", driverName);
    return false;
  }

  // Ensure the driver actually supports creating new files
  char** papszMetadata = poDriver->GetMetadata();
  if (!CSLFetchBoolean(papszMetadata, GDAL_DCAP_CREATE, FALSE)) {
    fLogSevere("GdalLatLonGrids: Driver '{}' does not support direct file creation.", driverName);
    return false;
  }

  // 4. Extract LatLonGrid Metadata
  int num_x = latLonGrid->getNumLons();
  int num_y = latLonGrid->getNumLats();
  float dx  = latLonGrid->getLonSpacing();
  float dy  = latLonGrid->getLatSpacing();
  
  auto loc = latLonGrid->getTopLeftLocationAt(0, 0);
  float nw_lat = loc.getLatitudeDeg();
  float nw_lon = loc.getLongitudeDeg();

  // 5. Create the GDAL Dataset
  // We specify GDT_Float32 since rapio uses floats for its primary data arrays
  GDALDataset* poDstDS = poDriver->Create(filepath.c_str(), num_x, num_y, 1, GDT_Float32, nullptr);
  if (poDstDS == nullptr) {
    fLogSevere("GdalLatLonGrids: Failed to create output file: {}", filepath);
    return false;
  }

  // 6. Set the GeoTransform (Origin and Spacing)
  double adfGeoTransform[6];
  adfGeoTransform[0] = nw_lon;
  adfGeoTransform[1] = dx;
  adfGeoTransform[2] = 0.0;        // No rotation
  adfGeoTransform[3] = nw_lat;
  adfGeoTransform[4] = 0.0;        // No rotation
  adfGeoTransform[5] = -dy;        // CRITICAL: GDAL expects negative Y-spacing!
  poDstDS->SetGeoTransform(adfGeoTransform);

  // 7. Set the Spatial Reference System (EPSG:4326 for standard Geographic Lat/Lon)
  OGRSpatialReference oSRS;
  oSRS.SetWellKnownGeogCS("WGS84");
  char* pszSRS_WKT = nullptr;
  oSRS.exportToWkt(&pszSRS_WKT);
  poDstDS->SetProjection(pszSRS_WKT);
  CPLFree(pszSRS_WKT);

  // 8. Extract the Raster Band and write the Data
  GDALRasterBand* poBand = poDstDS->GetRasterBand(1);
  poBand->SetNoDataValue(static_cast<double>(Constants::MissingData));

  auto array = latLonGrid->getFloat2D(Constants::PrimaryDataName);
  auto& data = array->ref();

#if 1
  // Write row-by-row to safely traverse the Boost multi_array
  for (int j = 0; j < num_y; ++j) {
    CPLErr err = poBand->RasterIO(GF_Write, 
                                  0, j, num_x, 1,             // Write X, Y, Width, Height
                                  &data[j][0], num_x, 1,      // CRITICAL FIX: raw pointer to row start
                                  GDT_Float32,                // Source Data Type
                                  0, 0);
    if (err != CE_None) {
      fLogSevere("GdalLatLonGrids: RasterIO write error at row {}", j);
      GDALClose(poDstDS);
      return false;
    }
  }
#else
  // Advanced Optimization: Writing the whole grid at once (optional)
  // Need to test
  CPLErr err = poBand->RasterIO(GF_Write, 
                              0, 0, num_x, num_y,           // Write full X, Y, Width, Height
                              &data[0][0], num_x, num_y,    // Pass pointer to absolute start of contiguous block
                              GDT_Float32, 
                              0, 0);
#endif


  // 9. Clean up and flush to disk
  GDALClose(poDstDS);
  fLogInfo("GDAL writer: Successfully wrote {} via {} driver.", filepath, driverName);

  return true;
}

