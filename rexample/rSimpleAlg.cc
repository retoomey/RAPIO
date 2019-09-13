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
      auto uplwav_ext = grib2->get2DData("ULWRF", "surface");

      if (uplwav_ext != nullptr) {
        LogInfo("First 10x10 values\n");
        LogInfo("----------------------------------------------------\n");
        // Dump grid.. Log needs a way to dump multiline
        for (size_t x = 0; x < 10; ++x) {
          for (size_t y = 0; y < 10; ++y) {
            std::cout << uplwav_ext->get(x, y) << ",  ";
          }
          std::cout << "\n";
        }
        LogInfo("----------------------------------------------------\n");
      } else {
        LogSevere("Couldn't read 2D data from grib2\n");
      }
      return; // We can't write grib2 yet
    }

    // Look for a radial set
    auto radialSet = d.datatype<rapio::RadialSet>();
    if (radialSet != nullptr) {
      // LogInfo("This is a radial set, do radial set stuff\n");
    }

    // Standard echo of data to output.  Note it's the same data out as in here
    LogInfo("--->Echoing " << r->getTypeName() << " product to output\n");
    writeOutputProduct(r->getTypeName(), *r); // Typename will be replaced by -O filters
  }
} // W2SimpleAlg::processNewData
