#include <rGribExample.h>

#include <iostream>

using namespace rapio;

/** A Grib2 reading example
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
GribExampleAlg::processNewData(rapio::RAPIOData& d)
{
  // Look for Grib2 data only
  auto grib2 = d.datatype<rapio::GribDataType>();

  if (grib2 != nullptr) {
    LogInfo("Grib2 data incoming...testing...\n");
    // ------------------------------------------------------------------------
    // Catalog test doing a .idx or wgrib2 listing attempt
    //
    {
      ProcessTimer grib("Testing print catalog speed\n");
      grib2->printCatalog();
      LogInfo("Finished in " << grib << "\n");
    }

    // ------------------------------------------------------------------------
    // 3D test
    //
    // This gets back layers into a 3D grid, which can be convenient for
    // certain types of data.  Also if you plan to work only with the data numbers
    // and don't care about transforming/projection say to mrms output grids.
    // Note that the dimensions have to match for all layers given
    const std::string name3D = "TMP";
    const std::vector<std::string> layers = { "2 mb", "5 mb", "7 mb" };

    LogInfo("Trying to read '" << name3D << "' as direct 3D from data...\n");
    auto array3D = grib2->getFloat3D(name3D, layers);

    if (array3D != nullptr) {
      LogInfo("Found '" << name3D << "'\n");
      LogInfo("Dimensions: " << array3D->getX() << ", " << array3D->getY() << ", " << array3D->getZ() << "\n");
    } else {
      LogSevere("Couldn't get 3D '" << name3D << "' out of grib data\n");
    }

    // ------------------------------------------------------------------------
    // 2D test
    //
    const std::string name2D = "ULWRF";
    const std::string layer  = "surface";

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
      }

      return;

      // ------------------------------------------------
      // ALPHA: Create projection lookup mapping
      // FIXME: Ok projection needs a LOT more work we need info
      // from the grib message on source projection, etc.

      // Create a brand new LatLonGrid
      float lat_spacing     = .01;
      float lon_spacing     = .01;
      const size_t num_lats = 3500; // 3500 conus
      const size_t num_lons = 7000; // 7000 conus
      auto llgridsp         = LatLonGrid::Create(
        name2D,
        "dimensionless",         // wrong probably
        LLH(55.0, -130.0, .500), // CONUS origin
        Time::CurrentTime(),     // Now
        lat_spacing,
        lon_spacing,
        num_lats, // X
        num_lons  // Y
      );

      // ALPHA: For moment just make one without caching or anything.
      // This is slow on first call, but we'd be able to cache in a
      // real time situation.
      // Also PROJ6 and up uses different strings
      // But basically we'll 'declare' our projection somehow
      Project * project = new ProjLibProject(
        "+proj=lcc +lon_0=-98 +lat_0=38 +lat_1=33 +lat_2=45 +x_0=0 +y_0=0 +units=km +resolution=3",
        // WDSSII default (probably doesn't even need to be parameter)
        "+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs"
      );
      bool success = project->initialize();

      LogInfo("Created projection:  " << success << "\n");
      // Project from source project to output
      project->toLatLonGrid(array2D, llgridsp);

      // Typename will be replaced by -O filters
      writeOutputProduct(llgridsp->getTypeName(), llgridsp);
    } else {
      LogSevere("Couldn't read 2D data '" << name2D << "' from grib2.\n");
    }
  }
} // GribExampleAlg::processNewData

int
main(int argc, char * argv[])
{
  // Run this thing standalone.
  GribExampleAlg alg = GribExampleAlg();

  alg.executeFromArgs(argc, argv);
}
