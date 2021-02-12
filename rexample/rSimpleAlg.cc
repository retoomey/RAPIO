#include <rSimpleAlg.h>
#include <boost/log/trivial.hpp>

#include <iostream>

using namespace rapio;

// Your group/project/area for all of YOUR code.
// Here we're part of wdssii or hmet or anc, etc.
using namespace wdssii;

/** A RAPIO Algorithm Example
 * Real time Algorithm Parameters and I/O
 * http://github.com/retoomey/RAPIO
 *
 * This is an example/template of creating a RAPIO algorithm.
 * I will try to enhance this example/modify it as the API evolves.
 *
 * @author Robert Toomey
 **/
void
W2SimpleAlg::declareOptions(RAPIOOptions& o)
{
  // NOTE: the i, I, l, O and r options are already defined in RAPIOAlgorithm.
  // i --> input index/url support
  // I --> input filter support default matches.
  // l --> notifier support (FAM files basically)
  // r --> realtime flag support
  // o.setDescription("WDSS2"); // Default for WDSS2/MRMS algorithms that you intend to copyright as part of WDSS2
  // o.setDescription("W2SimpleAlg does something");
  // o.setAuthors("Robert Toomey");

  // An optional string param...default is "Test" if not set by user
  // o.optional("T", "Test", "Test option flag");

  // An optional boolean param, since boolean defaults are always false.
  // o.boolean("x", "Turn x on and off for some reason.  Basically add a -x to turn on a boolean flag that is false by default");

  // A required parameter (algorithm won't run without it).  Here there is no default since it's required, instead you can provide an example of the setting
  // o.require("Z", "method1", "Set this to anything, it's just an example");
}

/** RAPIOAlgorithms process options on start up */
void
W2SimpleAlg::processOptions(RAPIOOptions& o)
{
  // This is an example of how to get your algorithm parameters
  // Stick them in instance variables you can use them later in processing.

  /*
   * myTest = o.getString("T");
   * myX = o.getBoolean("x");
   * myZ = o.getString("Z");
   * std::string xAsString = o.getString("x");
   * LogInfo(" ************************x IS " << myX << " (as string "<< xAsString << "\n");
   * LogInfo(" ************************T IS " << myTest << "\n");
   * LogInfo(" ************************Z IS " << myZ << "\n");
   */
}

void
W2SimpleAlg::processNewData(rapio::RAPIOData& d)
{
  // Use d.record() and d.datatype() to get your wdssii information...
  // We'll probably add more helper features in the API as it grows.
  // Note: You don't have print this in your algorithm, it's just
  // an example
  std::string s = "Data received: ";
  auto sel      = d.record().getSelections();
  for (auto& s1:sel) {
    s = s + s1 + " ";
  }
  LogInfo(s << " from index " << d.matchedIndexNumber() << "\n");

  // Example of reading data.  Typically if you know what you want,
  // you can basically:
  // auto myData = d.datatype<TYPE>();
  // if (myData != nullptr){
  //    // Do stuff
  // }
  // I'm hoping to add to API, maybe give a way to list out all
  // available datatypes supported.  Right now we
  // have RadialSet, LatLonGrid, GribDataType

  // Look for any data the system knows how to read
  auto r = d.datatype<rapio::DataType>();
  if (r != nullptr) {
    #if 0
    // Example for processing groups of subtypes.

    // Say you have data incoming where you
    // require N moments to all exist in order to process them.  For example, you need
    // 01.80 Reflectivity and 01.80 Velocity together to process your algorithm.

    // First save to a collection of virtual volumes for each subtype:
    DataTypeHistory::updateVolume(r); // Could save some memory just calling for your wanted moments

    // Looking for an existing slice across volume cache.  Basically if you need to process N moments
    // as soon as it is 'full' you can call this.  You can delete the slice afterwards for a clean slate,
    // otherwise you'll get the same set again (expect for the newest added)
    // You'll either get 0 size or types.size(), no inbetween.
    const std::vector<std::string> types = { "Reflectivity", "Velocity" }; // The types we must have...
    const std::string current = r->getSubType();                           // The current just added subtype
    auto list = DataTypeHistory::getFullSubtypeGroup(current, types);      // Get FULL matching subtype, each moment

    if (list.size() > 0) { // zero comes back if missing any of them.
      // Process list[0], list[1], etc.
      LogSevere(
        ">>>>>>>>>>>>>>>>FULL GROUP: PROCESSING REFLECTIVITY, VELOCITY AT SUBTYPE " << current
                                                                                    << "<<<<<<<<<<<<<<<<<<<<<<\n");
      ProcessMyNewAlg(list[0], list[1], list[2]) // These will match the 'types' in.
      // If processed now you can delete that slice if you don't want to chance processing this timeset again
      DataTypeHistory::deleteSubtypeGroup(current, types);
    }
    #endif // if 0

    // Look for Grib2 data for Grib2
    // std::shared_ptr<rapio::GribDataType> grib2 = d.datatype<rapio::GribDataType>();
    auto grib2 = d.datatype<rapio::GribDataType>();
    if (grib2 != nullptr) {
      LogInfo("Grib2 data incoming...testing...\n");
      // Test for moment for my wgrib2 style catalog listing
      grib2->printCatalog();

      // Grab the data for a particular grib2 record in style of an HMET
      // hrrr algorithm...
      // LogInfo("Reading 2m-air temperature from GRIB2...\n");
      // auto t2m_sfc_ext =  grib2->get2DData("TMP", "2 m above ground"); // /Degrees K
      // LogInfo("Reading dewpoint temperature from GRIB2...\n");
      // auto dpt_sfc_ext = grib2->get2DData("DPT", "2 m above ground");
      LogInfo("Reading upward longwave radiation product from GRIB2...\n");
      size_t numX     = 0;
      size_t numY     = 0;
      auto uplwav_ext = grib2->getFloat2D("ULWRF", "surface", numX, numY);
      LogInfo("Size back is " << numX << " by " << numY << "\n");

      if (uplwav_ext != nullptr) {
        auto& ref = uplwav_ext->ref();
        LogInfo("First 10x10 values\n");
        LogInfo("----------------------------------------------------\n");
        // Dump grid.. Log needs a way to dump multiline
        for (size_t x = 0; x < 10; ++x) {
          for (size_t y = 0; y < 10; ++y) {
            // std::cout << uplwav_ext->get(x, y) << ",  ";
            std::cout << ref[x][y] << ",  ";
            // std::cout << ref.get(x,y) << ",  ";
          }
          std::cout << "\n";
        }
        LogInfo("----------------------------------------------------\n");
      } else {
        LogSevere("Couldn't read 2D data from grib2\n");
      }

      // Create a completely NEW latlongrid for output in this case

      // ------------------------------------------------
      // ALPHA: Create projection lookup mapping

      // Create a brand new LatLonGrid
      float lat_spacing     = .01;
      float lon_spacing     = .01;
      const size_t num_lats = 3500; // 3500 conus
      const size_t num_lons = 7000; // 7000 conus
      auto llgridsp         = LatLonGrid::Create(
        "ULWRF",
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
      project->toLatLonGrid(uplwav_ext, llgridsp);

      /*
       *    // General LatLonGrid fill test
       *    // Get the 2D array from it.  FIXME: We should get x, y here I think.
       *    // Users can't just 'add' new dimensions, only same size (Restricted DataGrid)
       *    auto data2DF = llgridsp->getFloat2D("ULWRF");
       *
       *    float counter = 0;
       *    for (size_t x = 0; x < num_lats; ++x) {
       *      for (size_t y = 0; y < num_lons; ++y) {
       *        (*data2DF)[x][y] = counter;
       *        counter++;
       *      }
       *    }
       */

      // Write our LatLonGrid out.
      // Typename will be replaced by -O filters
      writeOutputProduct(llgridsp->getTypeName(), llgridsp);
      return;
    }

    // Look for a radial set

    auto radialSet = d.datatype<rapio::RadialSet>();
    if (radialSet != nullptr) {
      LogInfo("This is a radial set, do radial set stuff\n");
      size_t radials = radialSet->getNumRadials(); // x
      size_t gates   = radialSet->getNumGates();   // y
      auto& data     = radialSet->getFloat2D()->ref();
      for (size_t r = 0; r < radials; ++r) {
        for (size_t g = 0; g < gates; ++g) {
          data[r][g] = 5.0; // Replace every gate with 5 dbz
        }
      }
    }

    // Standard echo of data to output.  Note it's the same data out as in here
    LogInfo("--->Echoing " << r->getTypeName() << " product to output\n");
    writeOutputProduct(r->getTypeName(), r); // Typename will be replaced by -O filters
    LogInfo("--->Finished " << r->getTypeName() << " product to output\n");
  }
} // W2SimpleAlg::processNewData

void
W2SimpleAlg::processHeartbeat(const Time& n, const Time& p)
{
  LogInfo("Simple alg got a heartbeat...what do you want me to do?\n");
  // FIXME: longer example here maybe..
  // Some RadialSet I'm holding onto/modifying over time...now I write it every N time:
  // writeOutputProduct(r->getTypeName(), r); // Typename will be replaced by -O filters
}
