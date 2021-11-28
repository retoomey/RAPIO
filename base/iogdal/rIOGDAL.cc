#include "rIOGDAL.h"

#include "rFactory.h"
#include "rIOURL.h"
#include "rStrings.h"
#include "rProject.h"
#include "rColorMap.h"
#include "rGDALDataTypeImp.h"

// GDAL C++ library
#include "gdal_priv.h"

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
{ }

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

std::shared_ptr<DataType>
IOGDAL::readGDALDataType(const URL& url)
{
  std::vector<char> buf;
  IOURL::read(url, buf);

  if (!buf.empty()) {
    std::shared_ptr<GDALDataTypeImp> g = std::make_shared<GDALDataTypeImp>(buf);
    return g;
  } else {
    LogSevere("Couldn't read image datatype at " << url << "\n");
  }

  return nullptr;
} // IOGDAL::readGDALDataType

std::shared_ptr<DataType>
IOGDAL::createDataType(const std::string& params)
{
  // virtual to static
  return (IOGDAL::readGDALDataType(URL(params)));
}

bool
IOGDAL::encodeDataType(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>              & keys
)
{
  DataType& r = *dt;

  // DataType LLH to data cell projection
  auto project = r.getProjection(); // primary layer

  if (project == nullptr) {
    LogSevere("No projection ability for this datatype.\n");
    return false;
  }
  auto& p = *project;

  // Setup gdal
  static std::shared_ptr<ProjLibProject> project2;
  static bool setup = false;
  if (!setup) {
    GDALAllRegister(); // Register/introduce all GDAL drivers
    CPLPushErrorHandler(CPLQuietErrorHandler);
  }

  // ----------------------------------------------------------------------
  // Read settings
  //
  std::string filename = keys["filename"];
  if (filename.empty()) {
    LogSevere("Need a filename to output\n");
    return false;
  }
  std::string suffix = keys["suffix"];
  if (suffix.empty()) {
    suffix = ".tif";
  }
  std::string driver = keys["GTiff"];
  if (driver.empty()) {
    driver = "GTiff";
  }

  // Box settings from given BBOX string
  size_t rows;
  size_t cols;
  double top      = 0;
  double left     = 0;
  double bottom   = 0;
  double right    = 0;
  auto proj = p.getBBOX(keys, rows, cols, left, bottom, right, top);

  // FIXME: still need to cleanup suffix stuff
  if (keys["directfile"] == "false") {
    // We let writers control final suffix
    filename         = filename + "." + suffix;
    keys["filename"] = filename;
  }

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
      LogSevere("Unrecognized settings, using defaults\n");
    }
  }

  if (!optionSuccess) {
    return false;
  }
  #endif // if 0

  // ----------------------------------------------------
  LogInfo("GDAL writer settings: (suffix: " << suffix << ", driver: " << driver << "\n");

  /*
   * static bool setup = false;
   * if (!setup) {
   *  GDALAllRegister(); // Register/introduce all GDAL drivers
   *  CPLPushErrorHandler(CPLQuietErrorHandler);
   *  setup = true;
   * }
   */

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
    LogSevere("Couldn't load GDAL driver for '" << driver << "', cannot write output\n");
    // FIXME: Clean up GDAL stuff?
    return false;
  }

  // Does this driver support writing...otherwise we abort..
  char ** meta;
  meta = driverGeotiff->GetMetadata();
  if (CSLFetchBoolean(meta, GDAL_DCAP_CREATE, FALSE)) { } else {
    LogSevere("GDAL Driver '" << driver << "' does not have create/output ability, cannot write output\n");
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
    LogSevere("Transform failed? " << terr << "\n");
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
      LogSevere("GDAL ERROR during write detected\n"); // fixme count and not spam
    }
    GDALClose(geotiffDataset);
    //   CPLFree(rowBuff);

    // IODataType::generateRecord(dt, aURL, "gdal", records);
    // GDALDestroyDriverManager(); leaking?
    return true;
  }catch (const std::exception& e)
  {
    LogSevere("Exception write testing image output " << e.what() << "\n");
  }

  return false;
} // IOGDAL::encodeDataType
