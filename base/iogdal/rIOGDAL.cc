#include "rIOGDAL.h"

#include "rFactory.h"
#include "rIOURL.h"
#include "rStrings.h"
#include "rProject.h"
#include "rRadialSetLookup.h"
#include "rColorMap.h"
#include "rGDALDataTypeImp.h"

// GDAL C++ library
#include "gdal_priv.h"

#include "rRadialSet.h"

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
IOGDAL::createDataType(const URL& path)
{
  // virtual to static
  return (IOGDAL::readGDALDataType(path));
}

bool
IOGDAL::encodeDataType(std::shared_ptr<DataType> dt,
  const URL                                      & aURL,
  std::shared_ptr<XMLNode>                       dfs)
{
  // Parse settings.  If we're doing multi call we'll have no
  // choice but to read each time?
  if (dfs != nullptr) {
    try{
      /*
       *    auto output = dfs->getChild("output");
       *    auto flag = output.getAttr("test", std::string(""));
       *    if (flag == "true"){
       *      LogSevere("GDAL reader got settings\n");
       *    }else{
       *      LogSevere("GDAL reader did not get setting\n");
       *    }
       */
    }catch (const std::exception& e) {
      LogSevere("Unrecognized settings, using defaults\n");
    }
  }

  static bool setup = false;
  if (!setup) {
    GDALAllRegister(); // Register/introduce all GDAL drivers
    CPLPushErrorHandler(CPLQuietErrorHandler);
    setup = true;
  }

  // FIXME: cleanup.  Messy for now, just do RadialSets
  // FIXME: Don't we just need a projection lat lon method per DataType?
  // Not always, might be writing out stuff that isn't projected...
  std::shared_ptr<RadialSet> radialSet = std::dynamic_pointer_cast<RadialSet>(dt);
  if (!radialSet) {
    LogSevere("GDAL writer only handles RadialSet data at the moment\n");
    return false;
  }

  // FIXME: Refactoring xml settings/etc coming soon I think.  Hardcode here
  size_t cols      = 500;
  size_t rows      = 500;
  size_t degreeOut = 10.0; // degrees lat/lon from center of radar for box

  RadialSet& r = *radialSet;
  // FIXME: API needs to be better.  Humm maybe just default name is primary internally?
  // Does this possibly allow hosting smart ptr to leave scope in optimization?
  auto& data    = radialSet->getFloat2D("primary")->ref();
  auto myLookup = RadialSetLookup(r); // Bin azimuths.

  // Read it once for first test.  Yay
  static auto test = ColorMap::readColorMap("/home/dyolf/Reflectivity.xml");

  // Calculate the lat lon 'box' of the image using sizes, etc.
  // Kinda of a pain actually.
  const auto centerLLH = r.getRadarLocation();

  // DataType: getLatLonCoverage?  (default for RadialSet could be the degree thing?)

  // North America.  FIXME: General lat/lon box creator function
  // Latitude decreases as you go south
  // Longitude decreases as you go east
  // Ok we need a fixed aspect for non-square images...use the
  // long for the actual width
  auto lon      = centerLLH.getLongitudeDeg();
  auto left     = lon - degreeOut;
  auto right    = lon + degreeOut;
  auto width    = right - left;
  auto deltaLon = width / cols;

  // To keep aspect ratio per cell, use deltaLon to back calculate
  auto deltaLat = -deltaLon; // keep the same square per pixel
  auto lat      = centerLLH.getLatitudeDeg();
  auto top      = lat - (deltaLat * rows / 2.0);

  // LogSevere("DeltaLat/long " << deltaLat << ", " << deltaLon << "\n");
  // exit(1);

  //  LogSevere("Center:" << lat << ", " << lon << "\n");
  //  LogSevere("TopLeft:" << top << ", " << left << "\n");
  //  LogSevere("BottomRight:" << bottom << ", " << right << "\n");
  //  LogSevere("Delta values " << deltaLat << ", " << deltaLon << "\n");

  // Isn't this leaking?
  std::string outfile = aURL.toString() + ".tif";
  float * rowBuff     = (float *) CPLMalloc(sizeof(float) * cols);

  GDALDriver * driverGeotiff = GetGDALDriverManager()->GetDriverByName("GTiff");
  if (driverGeotiff == nullptr) {
    // Can't load?
  }

  // Does this driver support writing...otherwise we abort..
  char ** meta;
  meta = driverGeotiff->GetMetadata();
  if (CSLFetchBoolean(meta, GDAL_DCAP_CREATE, FALSE)) { } else {
    // NOT supported
  }
  //
  // CSLFetchBoolean(meta, GDAL_DCAP_CREATE, FALSE):

  GDALDataset * geotiffDataset = driverGeotiff->Create(outfile.c_str(), cols, rows, 1, GDT_Float32, NULL);
  // Data band is all we have.
  geotiffDataset->GetRasterBand(1)->SetNoDataValue(Constants::MissingData);
  // WDSS2 uses color map to set red, green, blue..which is nonsense for color mapped data.
  // It would be correct for satellite visual frequencies

  // Projection system
  OGRSpatialReference oSRS;
  oSRS.SetWellKnownGeogCS("WGS84");

  // Transformation
  double geoTransform[6] = { left, deltaLon, 0, top, 0, deltaLat };
  CPLErr terr = geotiffDataset->SetGeoTransform(geoTransform);
  if (terr != 0) {
    LogSevere("Transform failed? " << terr << "\n");
  }

  // What is this doing?
  char * pszSRS_WKT = 0;
  oSRS.exportToWkt(&pszSRS_WKT);
  geotiffDataset->SetProjection(pszSRS_WKT);
  CPLFree(pszSRS_WKT);

  geotiffDataset->SetMetadataItem("TIFFTAG_SOFTWARE", "RAPIO C++ library");

  // Set tiff time to our data time
  const std::string tiffTime = dt->getTime().getString("%Y:%m:%d %H:%M:%S");
  geotiffDataset->SetMetadataItem("TIFFTAG_DATETIME", tiffTime.c_str());

  try{
    size_t nx = cols;
    size_t ny = rows;
    float azDegs, rangeMeters;
    int radial, gate;

    // Old school vslice/cappi, etc...where we hunt in the data space
    // using the coordinate of the destination.
    auto startLat = top;
    for (size_t y = 0; y < ny; y++) {
      auto startLon = left;
      for (size_t x = 0; x < nx; x++) {
        // Project Lat Lon to closest Az Range, then lookup radial/gate
        Project::LatLonToAzRange(lat, lon, startLat, startLon, azDegs, rangeMeters);
        bool good = myLookup.getRadialGate(azDegs, rangeMeters, &radial, &gate);

        // Color c = cm(v); bleh creating a color object like this has to be slower
        double v = good ? data[radial][gate] : Constants::MissingData;

        // Current color mapping... FIXME: Full color map ability from wg2 ported next
        rowBuff[x] = v; // super native attempt

        /*
         *    if (v == Constants::MissingData){
         * pixel = Magick::Color("blue");
         *    }else{
         *          // Ok let's linearly map red to 'strength' of DbZ for POC
         *      // working at moment with any fancy color mapping
         *      // Magick::ColorRGB cc = Magick::ColorRGB(c.r/255.0, c.g/255.0, c.b/255.0);
         *      // FIXME: Flush out color map ability
         *      unsigned char r, g, b, a;
         * if (test != nullptr){
         *      test->getColor(v, r,g,b,a);
         * }else{
         *      if (v > 200) v = 200;
         *          if (v < -50.0) v = -50.0;
         *      float weight = (250-v)/250.0;
         *      }
         *           // FIXME: transparent ability? if (transparent) {cc.alpha((255.0-c.a)/255.0); }
         *    }
         */
        startLon += deltaLon;
      }
      CPLErr err = geotiffDataset->GetRasterBand(1)->RasterIO(GF_Write, 0, y, cols, 1, rowBuff, cols, 1, GDT_Float32, 0,
          0);
      if (err != 0) {
        LogSevere("GDAL ERROR during write detected\n"); // fixme count and not spam
      }
      startLat += deltaLat;
    }
    GDALClose(geotiffDataset);
    // GDALDestroyDriverManager(); leaking?

    // FIXME: Bleh this uses the suffix of the path to determine type.
    // That will actually be good if we update the config ability
    // FIXME: Generalize and optimize the 'dfs' setting xml
    // FIXME: Do we write ourselves then compress, etc. or let this library
    // handle that directly here.
    //    i.write(aURL.toString()+".tif");
  }catch (const std::exception& e)
  {
    LogSevere("Exception write testing image output " << e.what() << "\n");
  }

  return false;
} // IOGDAL::encodeDataType
