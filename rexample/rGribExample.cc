#include <rGribExample.h>

#include <iostream>

using namespace rapio;

/** A Grib2 reading example
 *
 * Using hrrr data (has the fields used in example):
 * https://www.nco.ncep.noaa.gov/pmb/products/hrrr/
 * rgribexample -i file=hrrr.t00z.wrfnatf29.grib2 -o=test
 *
 * Using rrfs data:
 * rgribexample -i file=rrfs.t23z.prslev.f000.conus_3km.grib2 -o=test
 *
 * @author Robert Toomey
 **/
void
GribExampleAlg::declareOptions(RAPIOOptions& o)
{
  o.setDescription("Grib example");
  o.setAuthors("Robert Toomey");
}

void
GribExampleAlg::processOptions(RAPIOOptions& o)
{ }

void
GribExampleAlg::testGet2DArray(std::shared_ptr<GribDataType> grib2)
{
  LogInfo("-----Running get2DArray test ------------------------------------------\n");
  // ------------------------------------------------------------------------
  // 2D test
  //
  const std::string name2D = "TMP";
  const std::string layer  = "100 mb";

  LogInfo("Trying to read " << name2D << " as direct 2D from data...\n");
  auto array2D = grib2->getFloat2D(name2D, layer);

  if (array2D != nullptr) {
    LogInfo("Found '" << name2D << "'\n");
    LogInfo("Dimensions: " << array2D->getX() << ", " << array2D->getY() << "\n");

    auto& ref = array2D->ref(); // or (*ref)[x][y]
    LogInfo("First 10x10 values:\n");
    LogInfo("----------------------------------------------------\n");
    for (size_t x = 0; x < 10; ++x) {
      for (size_t y = 0; y < 10; ++y) {
        std::cout << ref[x][y] << ",  ";
      }
      std::cout << "\n";
    }
    LogInfo("----------------------------------------------------\n");
  } else {
    LogSevere("Couldn't getFloat2D '" << name2D << "' from grib2.\n");
  }
}

void
GribExampleAlg::testGet3DArray(std::shared_ptr<GribDataType> grib2)
{
  LogInfo("-----Running get3DArray test ------------------------------------------\n");
  // ------------------------------------------------------------------------
  // 3D test
  //
  // This gets back layers into a 3D grid, which can be convenient for
  // certain types of data.  Also if you plan to work only with the data numbers
  // and don't care about transforming/projection say to mrms output grids.
  // Note that the dimensions have to match for all layers given
  const std::string name3D = "TMP";
  // const std::vector<std::string> layers = { "16 hybrid level", "17 hybrid level", "18 hybrid level" };
  const std::vector<std::string> layers = { "100 mb", "125 mb", "150 mb" };

  LogInfo("Trying to read '" << name3D << "' as direct 3D from data...\n");
  auto array3D = grib2->getFloat3D(name3D, layers);

  if (array3D != nullptr) {
    LogInfo("Found '" << name3D << "'\n");
    LogInfo("Dimensions: " << array3D->getX() << ", " << array3D->getY() << ", " << array3D->getZ() << "\n");

    // Print 3D layer 0 which should match the 2D called before, right?
    auto& ref = array3D->ref(); // or (*ref)[x][y]
    LogInfo("First 10x10 values:\n");
    LogInfo("----------------------------------------------------\n");
    for (size_t x = 0; x < 10; ++x) {
      for (size_t y = 0; y < 10; ++y) {
        // FIXME: LatLonHeightGrids store in layer, lat, lon.  But array
        // at the moment we're using lat, lon, layer.  'Maybe' we sync
        // the ordering at some point.  Might not matter.
        std::cout << ref[x][y][0] << ",  ";
      }
      std::cout << "\n";
    }
    LogInfo("----------------------------------------------------\n");
  } else {
    LogSevere("Couldn't get 3D '" << name3D << "' out of grib data\n");
  }
} // GribExampleAlg::testGet3DArray

void
GribExampleAlg::testGetMessageAndField(std::shared_ptr<GribDataType> grib2)
{
  LogInfo("-----Running getMessage and getField test -----------------------------\n");

  const std::string name2D = "TMP";
  const std::string layer  = "surface";

  LogInfo("Trying to read " << name2D << " as grib message.\n");
  auto message = grib2->getMessage(name2D, layer);

  if (message != nullptr) {
    LogInfo("Success with higher interface..read message '" << name2D << "'\n");
    // FIXME: add methods for various things related to messages
    // and possibly fields.  We should be able to query a field
    // off a message at some point.
    // FIXME: Enums if this stuff is needed/useful..right now most just
    // return numbers which would make code kinda unreadable I think.
    // message->getFloat2D(optional fieldnumber == 1)
    auto& m = *message;
    LogInfo("    Time of the message is " << m.getDateString() << "\n");
    LogInfo("    Message in file is number " << m.getMessageNumber() << "\n");
    LogInfo("    Message file byte offset: " << m.getFileOffset() << "\n");
    LogInfo("    Message byte length: " << m.getMessageLength() << "\n");
    // Time theTime = message->getTime();
    LogInfo("    Center ID is " << m.getCenterID() << "\n");
    LogInfo("    SubCenter ID is " << m.getSubCenterID() << "\n");

    // Messages have (n) fields each
    auto field = message->getField(1);
    if (field != nullptr) {
      auto& f = *field;
      LogInfo("For field 1 of the message:\n");
      LogInfo("    GRIB Version: " << f.getGRIBEditionNumber() << "\n");
      LogInfo("    GRIB Discipline #: " << f.getDisciplineNumber() << "\n");
      LogInfo("    Time of the field is " << f.getDateString() << "\n");
      LogInfo("    Grid def template number is: " << f.getGridDefTemplateNumber() << "\n");
    }
  }   else {
    LogSevere("Couldn't getMessage '" << name2D << "' from grib2.\n");
  }
} // GribExampleAlg::testGetMessageAndField

void
GribExampleAlg::testGetLatLonGrid(std::shared_ptr<GribDataType> grib2)
{
  LogInfo("-----Running getLatLonGrid test ---------------------------------------\n");
  const std::string name2D = "TMP";
  const std::string layer  = "surface";

  auto llg = grib2->getLatLonGrid(name2D, layer);

  if (llg != nullptr) {
    // FIXME: Question is do we make some sort of look up table or what to turn
    // grib2 names into MRMS ones.
    llg->setTypeName("Temperature");
    llg->setUnits("K");

    writeOutputProduct(llg->getTypeName(), llg);
  } else {
    LogSevere("Was unable to getLatLonGrid matching '" << name2D << " " << layer << "'\n");
  }
}

void
GribExampleAlg::processNewData(RAPIOData& d)
{
  // Look for Grib2 data only
  auto grib2 = d.datatype<GribDataType>();

  if (grib2 != nullptr) {
    LogInfo("Grib2 data incoming...testing...\n");

    // Dump catalog
    grib2->printCatalog();

    testGet2DArray(grib2);
    testGet3DArray(grib2);
    testGetMessageAndField(grib2);
    testGetLatLonGrid(grib2);

    #if 0
    // All of this was experiment with projection.  The new iowgrib2 module will use
    // the wgrib2 projection.  Might still play with this stuff at some point.
    // having internal rapio projection stuff is useful but then we have to maintain it
    // vs having other libraries do their internal stuff, but then we don't have
    // generic ability.

    // Create a brand new LatLonGrid
    size_t num_lats, num_lons;

    // Raw copy for orientation test.  Good for checking data orientation correct
    const bool rawCopy = false;
    if (rawCopy) {
      num_lats = array2D->getX();
      num_lons = array2D->getY();
    } else {
      num_lats = 1750;
      num_lons = 3500;
    }

    // Reverse spacing from the cells and range
    // FIXME: We could have another create method using degrees I think
    float lat_spacing = (55.0 - 10.0) / (float) (num_lats); // degree spread/cell count
    float lon_spacing = (130.0 - 60.0) / (float) num_lons;

    auto llgridsp = LatLonGrid::Create(
      "SurfaceTemp",           // MRMS name and also colormap
      "degreeK",               // Units
      LLH(55.0, -130.0, .500), // CONUS origin
      message->getTime(),
      lat_spacing,
      lon_spacing,
      num_lats, // X
      num_lons  // Y
    );

    // Just copy to check lat/lon orientation
    if (rawCopy) {
      auto& out   = llgridsp->getFloat2DRef();
      size_t numX = array2D->getX();
      size_t numY = array2D->getY();
      LogSevere("OUTPUT NUMX NUMY " << numX << ", " << numY << "\n");
      for (size_t x = 0; x < numX; ++x) {
        for (size_t y = 0; y < numY; ++y) {
          out[x][y] = ref[x][y];
        }
      }
    } else {
      // ------------------------------------------------
      // ALPHA: Create projection lookup mapping
      // FIXME: Ok projection needs a LOT more work we need info
      // from the grib message on source projection, etc.
      // ALPHA: For moment just make one without caching or anything.
      // This is slow on first call, but we'd be able to cache in a
      // real time situation.
      // Also PROJ6 and up uses different strings
      // But basically we'll 'declare' our projection somehow
      Project * project = new ProjLibProject(
        // axis: Tell project that our data is east and south heading
        "+proj=lcc +axis=esu +lon_0=-98 +lat_0=38 +lat_1=33 +lat_2=45 +x_0=0 +y_0=0 +units=km +resolution=3"
      );
      bool success = project->initialize();
      LogInfo("Created projection:  " << success << "\n");
      // Project from source project to output
      if (success) {
        project->toLatLonGrid(array2D, llgridsp);
      } else {
        LogSevere("Failed to create projection\n");
        return;
      }
    }
    // Typename will be replaced by -O filters
    writeOutputProduct(llgridsp->getTypeName(), llgridsp);
  } else {
    LogSevere("Couldn't read 2D data '" << name2D << "' from grib2.\n");
  }
    #endif // if 0
  }
} // GribExampleAlg::processNewData

int
main(int argc, char * argv[])
{
  // Run this thing standalone.
  GribExampleAlg alg = GribExampleAlg();

  alg.executeFromArgs(argc, argv);
}
