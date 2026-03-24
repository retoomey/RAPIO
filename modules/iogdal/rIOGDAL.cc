#include "rIOGDAL.h"

#if 0
#include "rFactory.h"
#include "rIOURL.h"
#include "rStrings.h"
#include "rProject.h"
#include "rColorMap.h"
#endif
#include "rFactory.h"
#include "rGDALDataTypeImp.h"
#include "rGDALLatLonGrids.h" // Handle standard grids

// GDAL C++ library
#include "gdal_priv.h"

#if 0
// GDAL simple features
#include "ogr_api.h"
#include "ogrsf_frmts.h"
#include "ogr_p.h"
#endif
#include "ogr_api.h"
#include "ogrsf_frmts.h"
#include "ogr_p.h"

using namespace rapio;

// Library dynamic link to create this factory
extern "C"
{
void *
createRAPIOIO(void)
{
  auto * z = new IOGDAL();

  z->initialize();
  return reinterpret_cast<void *>(z);
}
};

std::string
IOGDAL::getHelpString(const std::string& key)
{
  std::string help;

  help += "builder that uses the GDAL library and supported formats.";
  return help;
}

void
IOGDAL::initialize()
{
  GDALAllRegister(); // Register/introduce all GDAL drivers
  //CPLPushErrorHandler(CPLQuietErrorHandler);
  // Register the specializers we currently support
  GDALLatLonGrids::introduceSelf(this);
}

IOGDAL::~IOGDAL()
{ }

void
IOGDAL::introduce(const std::string & name,
  std::shared_ptr<GDALType>         new_subclass)
{
  Factory<GDALType>::introduce(name, new_subclass);
}

std::shared_ptr<rapio::GDALType>
IOGDAL::getIOGDAL(const std::string& name)
{
  return (Factory<GDALType>::get(name, "GDAL builder"));
}

#if 0
std::shared_ptr<DataType>
IOGDAL::readGDALDataType(const URL& url)
{
  // Don't think there's a read from memory ability in GDAL, lots
  // of libraries fail to do this.  Maybe the C arrow stream interface,
  // but it's too new for now I think
  std::vector<char> buf;

  // IOURL::read(url, buf);

  //  if (!buf.empty()) {
  std::shared_ptr<GDALDataTypeImp> g = std::make_shared<GDALDataTypeImp>();

  fLogInfo("Sending {} to GDALOpenEx...", url.toString());
  bool success = g->readGDALDataset(url.toString());


  #if 0
  GDALDataset * poDS;

  // general GDALDataTypeImp right?
  // FIXME: How do we dispose of this thing?
  poDS = (GDALDataset *) GDALOpenEx(url.toString().c_str(), GDAL_OF_VECTOR, NULL, NULL, NULL);
  if (poDS == NULL) {
    fLogSevere("OK it didn't read... :(");
  } else {
    // If it's a raster right?  I'm getting raster size 512 * 512 for a shapefile?
    // So are all the raster bands in a GDAL file the same size?  We can use that to
    // map to DataGrid arrays...
    int xSize = poDS->GetRasterXSize();
    int ySize = poDS->GetRasterYSize();
    fLogSevere("Sizes of raster is {}, {}", xSize, ySize);
    int rcount = poDS->GetRasterCount();
    fLogSevere("Raster count is {}", rcount); // zero for shapefile...ok works
    for (auto&& poBand: poDS->GetBands() ) {
      //  poBand->GetDescription();
    }

    // Get the layers
    // I want to read a shapefile here for first pass,
    // but we could and probably should generally wrap a GDALDataset
    size_t numLayers = poDS->GetLayerCount();
    for (auto&& poLayer: poDS->GetLayers() ) { // from docs
      fLogSevere("Layer {}", poLayer->GetName());

      // Ok features are read and created it seems...
      OGRFeature * feature = NULL;
      poLayer->ResetReading();
      size_t i = 0;
      // In OK shapefile these are the counties...
      while ((feature = poLayer->GetNextFeature()) != NULL) {
        fLogSevere("{} Feature...", i);

        // Ok the fields are stuff stored per feature, like
        // phone number, etc.
        for (auto&& oField: *feature) {
          switch (oField.GetType()) {
              default:
                fLogSevere("FIELD VALUE: {}", oField.GetAsString());
                break;
          }
        }

        // The geometry of the object. FIXME: How to project to wsr84?
        // Each feature can have _several geometry fields..interesting
        OGRGeometry * poGeometry = feature->GetGeometryRef();

        int geoCount = feature->GetGeomFieldCount();
        fLogSevere("FEATURE HAS {}", geoCount);
        // Ahh kill me there's like 10 billion types here... wkbUnknown, wkbLineString
        // if (poGeometry != NULL && wkbFlatten(poGeometry->getGeometryType()) == wkbPoint )
        if (poGeometry != NULL) {
          fLogSevere("The type is {}", wkbFlatten(poGeometry->getGeometryType()));
        }
        // if (poGeometry != NULL && wkbFlatten(poGeometry->getGeometryType()) == wkbPoint )
        if ((poGeometry != NULL) && (wkbFlatten(poGeometry->getGeometryType()) == wkbPolygon) ) {
          # if GDAL_VERSION_NUM >= GDAL_COMPUTE_VERSION(2, 3, 0)
          // OGRPoint *poPoint = poGeometry->toPoint();
          OGRPolygon * poPoint = poGeometry->toPolygon();
          # else
          // OGRPoint *poPoint = (OGRPoint *)poGeometry;
          OGRPolygon * poPoint = (OGRPolygon *) poGeometry;
          # endif
          //        fLogSevere("POINT: {}{}", poPoint->getX(), poPoint->getY());

          // char * test = poGeometry->exportToJson();  Wow it works?
          ///  char * test = poGeometry->exportToKML();
          // poGeometry->GetPointCount();
          exit(1);
          // exportToJson()  exportToKML()  free with CPLFree()?
        } else {
          fLogSevere("No point geometry!");
        }


        OGRFeature::DestroyFeature(feature);
        i++;
      }

      // FIXME: I guess layers don't get deleted?
    }
    fLogSevere(">>>>GOT BACK {} layers", numLayers);

    // delete poDS; // One delete doesn't crash so guess we do it?
    // On windows they say call GDALClose()

    GDALClose(poDS);
  }
  #endif // if 0

  return success ? g : nullptr;
  // } else {
  //   fLogSevere("Couldn't read image datatype at {}", url.toString());
  // }
} // IOGDAL::readGDALDataType
#endif

std::shared_ptr<DataType>
IOGDAL::createDataType(const std::string& params)
{
  // virtual to static
//  return (IOGDAL::readGDALDataType(URL(params)));
//

  URL url(params);
  std::string filepath = url.toString();
  
  // Strip "gdal:" prefix if present (RAPIO already did this)
 // if (filepath.rfind("gdal:", 0) == 0) {
 //     filepath = filepath.substr(5);
 // }

  // 1. Ensure GDAL is registered
  GDALAllRegister();

  // 2. Open the Dataset to interrogate it
  GDALDataset* poDataset = static_cast<GDALDataset*>(GDALOpen(filepath.c_str(), GA_ReadOnly));
  if (poDataset == nullptr) {
    fLogSevere("IOGdal: Failed to open dataset at {}, GDAL Error: {}", filepath, CPLGetLastErrorMsg());
    return nullptr;
  }

  std::string target_specializer = "";

  // 3. The Interrogation / Dispatch Logic
  double adfGeoTransform[6];
  
  if (poDataset->GetRasterCount() > 0 && poDataset->GetGeoTransform(adfGeoTransform) == CE_None) {
      // Check for rotation/shearing (indices 2 and 4). LatLonGrid expects North-up.
      if (adfGeoTransform[2] == 0.0 && adfGeoTransform[4] == 0.0) {
          fLogInfo("IOGdal: Dataset identified as an unrotated 2D Grid.");
          target_specializer = "LatLonGrid";
      } else {
          fLogSevere("IOGdal: Dataset is a rotated raster. Not currently supported by LatLonGrid.");
      }
  } 
  // Future extensibility for Vector data (Shapefiles, GeoJSON)
  else if (poDataset->GetLayerCount() > 0) {
      fLogInfo("IOGdal: Dataset identified as Vector feature data.");
      // target_specializer = "VectorFeatureSet"; // When you build this class
      fLogSevere("IOGdal: Vector data routing not yet implemented.");
  }
  // Future extensibility for Subdatasets (HDF5, complex NetCDF)
  else if (poDataset->GetMetadata("SUBDATASETS") != nullptr) {
      fLogSevere("IOGdal: Dataset is a container of subdatasets. Routing not yet implemented.");
  } 
  else {
      fLogSevere("IOGdal: Dataset format recognized by GDAL, but does not map to any known rapio DataType.");
  }

  // 4. Close the dataset (the specializer will reopen it, or you can pass the pointer via keys if you prefer to optimize)
  GDALClose(poDataset);

  // 5. Dispatch to the specific specializer
  if (!target_specializer.empty()) {
      std::map<std::string, std::string> keys;
      keys["FilePath"] = filepath;
      
      std::shared_ptr<IOSpecializer> fmt = getIOSpecializer(target_specializer);
      if (fmt != nullptr) {
          return fmt->read(keys, nullptr);
      } else {
          fLogSevere("IOGdal: No specializer registered for type '{}'", target_specializer);
      }
  }

  return nullptr;
}

bool
IOGDAL::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>             & keys
)
{
  const std::string type = dt->getDataType();
  std::shared_ptr<IOSpecializer> fmt = getIOSpecializer(type);

  // Fallback: If it's a grid but doesn't have a direct GDAL specializer yet, 
  // try treating it as a generic LatLonGrid
  if (fmt == nullptr) {
    auto grid = std::dynamic_pointer_cast<LatLonGrid>(dt);
    if (grid != nullptr) {
      fmt = getIOSpecializer("LatLonGrid");
    }
  }

  if (fmt == nullptr) {
    fLogSevere("IOGDAL: Cannot create a GDAL IO writer for datatype {}", type);
    return false;
  }

  std::string filename;
  if (!resolveFileName(keys, "tif", "gdal-", filename)) {
    return false;
  }
  
  // The GDALLatLonGrids specializer expects the target path in the "FilePath" key
  keys["FilePath"] = filename;

  bool successful = false;
  try {
    // Route directly to GDALLatLonGrids::write (No BBOX, no manual looping!)
    successful = fmt->write(dt, keys);
  } catch (const std::exception& e) {
    fLogSevere("IOGDAL: Exception during GDAL write: {}", e.what());
    successful = false;
  }

  if (successful) {
    successful = postWriteProcess(filename, keys);
  }
  
  if (successful) {
    showFileInfo("GDAL writer: ", keys);
  }

  return successful;
}

#if 0
bool
IOGDAL::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>             & keys
)
{
  bool successful = true;

  DataType& r = *dt;

  // DataType LLH to data cell projection
  auto project = r.getProjection(); // primary layer

  if (project == nullptr) {
    fLogSevere("No projection ability for this datatype.");
    return false;
  }
  auto& p = *project;

  static std::shared_ptr<ProjLibProject> project2;

  // ----------------------------------------------------------------------
  // Read settings
  //

  // ----------------------------------------------------------
  // Get the filename we should write to
  std::string filename;

  if (!resolveFileName(keys, "tif", "gdal-", filename)) {
    return false;
  }

  // Get GDAL driver
  std::string driver = keys["GTiff"];

  if (driver.empty()) {
    driver = "GTiff";
  }

  // Box settings from given BBOX string
  size_t rows;
  size_t cols;
  double top    = 0;
  double left   = 0;
  double bottom = 0;
  double right  = 0;
  auto proj     = p.getBBOX(keys, rows, cols, left, bottom, right, top);

  // ----------------------------------------------------------------------
  // Calculate bounding box for generating image
  bool transform = (proj != nullptr);

  // std::string suffix = "tif";
  // std::string driver = "GTiff";
  #if 0
  bool optionSuccess = false;

  if (dfs != nullptr) {
    try{
      auto output = dfs->getChild("output");
      suffix        = output.getAttr("suffix", suffix);
      driver        = output.getAttr("driver", driver);
      optionSuccess = p.getLLCoverage(output, cover);
    }catch (const std::exception& e) {
      fLogSevere("Unrecognized settings, using defaults");
    }
  }

  if (!optionSuccess) {
    return false;
  }
  #endif // if 0

  // Pull back settings in coverage for marching

  /*
   * const size_t rows    = cover.rows;
   * const size_t cols    = cover.cols;
   * const float top      = cover.topDegs;
   * const float left     = cover.leftDegs;
   * const float deltaLat = cover.deltaLatDegs;
   * const float deltaLon = cover.deltaLonDegs;
   */

  // For the moment, always have a color map, even if missing...
  // Only needed if we are doing artificial color bands in the data such as in
  // the current MRMS RIDGE images

  /*
   * static std::shared_ptr<ColorMap> colormap;
   * if (colormap == nullptr){
   * colormap = ColorMap::readColorMap("/home/dyolf/Reflectivity.xml");
   * if (colormap == nullptr){
   *   colormap = std::make_shared<LinearColorMap>();
   * }
   * }
   * const ColorMap& test = *colormap;
   */

  // bool useSubDirs     = true;
  // URL aURL            = IODataType::generateFileName(dt, params, "tif", directFile, useSubDirs);
  // aURL.toString();
  std::string outfile = filename;

  GDALDriver * driverGeotiff = GetGDALDriverManager()->GetDriverByName(driver.c_str());
  if (driverGeotiff == nullptr) {
    // Can't load?
    fLogSevere("Couldn't load GDAL driver for '{}', connot write output", driver);
    // FIXME: Clean up GDAL stuff?
    return false;
  }

  // Does this driver support writing...otherwise we abort..
  char ** meta;
  meta = driverGeotiff->GetMetadata();
  if (CSLFetchBoolean(meta, GDAL_DCAP_CREATE, FALSE)) { } else {
    fLogSevere("GDAL Driver '{}' does not have create/output ability, cannot write output", driver);
    return false;
  }

  GDALDataset * geotiffDataset = driverGeotiff->Create(outfile.c_str(), cols, rows, 1, GDT_Float32, NULL);
  // Data band is all we have.
  // WDSS2 uses color map to set red, green, blue..which is nonsense for color mapped data.
  // It would be correct for satellite visual frequencies

  // Projection system
  OGRSpatialReference oSRS;
  oSRS.SetWellKnownGeogCS("WGS84");

  // Transformation of data from grid to actual coordinates
  auto deltaLat = (bottom - top) / rows;
  auto deltaLon = (right - left) / cols;
  double geoTransform[6] = { left, deltaLon, 0, top, 0, deltaLat };
  CPLErr terr = geotiffDataset->SetGeoTransform(geoTransform);
  if (terr != 0) {
    fLogSevere("Transform failed? {}", (int) terr);
  }

  // Set the geometric projection being used on this data
  char * pszSRS_WKT = 0;
  oSRS.exportToWkt(&pszSRS_WKT);
  geotiffDataset->SetProjection(pszSRS_WKT);
  CPLFree(pszSRS_WKT);

  // Metadata, set some known tiff tags
  geotiffDataset->SetMetadataItem("TIFFTAG_SOFTWARE", "RAPIO C++ library");
  const std::string tiffTime = dt->getTime().getString("%Y:%m:%d %H:%M:%S");
  geotiffDataset->SetMetadataItem("TIFFTAG_DATETIME", tiffTime.c_str());

  // ----------------------------------------------------------------------
  // Final rendering using the BBOX
  try{
    auto& band1 = *(geotiffDataset->GetRasterBand(1));
    band1.SetNoDataValue(Constants::MissingData);

    // Does this need to be a pointer or faster to use a vector on stack?
    //   float * rowBuff     = (float *) CPLMalloc(sizeof(float) * cols);
    // std::vector<float> rowBuff(cols);
    std::vector<float> rowBuff(cols * rows);
    size_t index  = 0;
    auto startLat = top;
    for (size_t y = 0; y < rows; y++) {
      auto startLon = left;
      for (size_t x = 0; x < cols; x++) {
        if (transform) {
          double aLat, aLon;
          project2->xyToLatLon(startLon, startLat, aLat, aLon);
          const double v = p.getValueAtLL(aLat, aLon);
          rowBuff[index++] = v; // Data value band
        } else {
          const double v = p.getValueAtLL(startLat, startLon);
          rowBuff[index++] = v; // Data value band
        }
        startLon += deltaLon;
      }
      // Write per row
      // CPLErr err = band1.RasterIO(GF_Write, 0, y, cols, 1, &rowBuff[0], cols, 1, GDT_Float32, 0, 0);
      startLat += deltaLat;
    }
    // Full write final grid
    CPLErr err = band1.RasterIO(GF_Write, 0, 0, cols, rows, &rowBuff[0], cols, rows, GDT_Float32, 0, 0);
    if (err != 0) {
      fLogSevere("GDAL ERROR during write detected"); // fixme count and not spam
    }
    GDALClose(geotiffDataset);
    //   CPLFree(rowBuff);

    // ----------------------------------------------------------
    // Post processing such as extra compression, ldm, etc.
    if (successful) {
      successful = postWriteProcess(filename, keys);
    }
    // IODataType::generateRecord(dt, aURL, "gdal", records);
    // GDALDestroyDriverManager(); leaking?
    // Standard output
    if (successful) {
      showFileInfo("GDAL writer: ", keys);
    }
  }catch (const std::exception& e)
  {
    fLogSevere("Exception write testing image output {}", e.what());
    successful = false;
  }

  return successful;
} // IOGDAL::encodeDataType
#endif
