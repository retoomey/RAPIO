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

    // Look for a radial set

    auto radialSet = d.datatype<rapio::RadialSet>();
    if (radialSet != nullptr) {
      LogInfo("This is a radial set, do radial set stuff\n");
      auto time = radialSet->getTime();
      time += TimeDuration::Days(18250);

      radialSet->setTime(time);


      auto newLocation = radialSet->getLocation();
      newLocation.setLatitudeDeg(35.3331);
      newLocation.setLongitudeDeg(-97.2778);
      newLocation.setHeightKM(.390);
      radialSet->setLocation(newLocation);
      radialSet->setUnits("dBZ");
      radialSet->setRadarName("KTLX");

      /*
       * size_t radials = radialSet->getNumRadials(); // x
       * size_t gates   = radialSet->getNumGates();   // y
       * auto& data     = radialSet->getFloat2D()->ref();
       * for (size_t r = 0; r < radials; ++r) {
       * for (size_t g = 0; g < gates; ++g) {
       *  data[r][g] = 5.0; // Replace every gate with 5 dbz
       * }
       * }
       */
    }

    // Standard echo of data to output.  Note it's the same data out as in here
    LogInfo("--->Echoing " << r->getTypeName() << " product to output\n");
    std::map<std::string, std::string> myOverrides;
    // myOverrides["postSuccessCommand"] = "ldm";            // Do a standard pqinsert of final data file
    writeOutputProduct(r->getTypeName(), r, myOverrides); // Typename will be replaced by -O filters
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

int
main(int argc, char * argv[])
{
  // Create your algorithm instance and run it.  Isn't this API better?
  W2SimpleAlg alg = W2SimpleAlg();

  // Run this thing standalone.
  alg.executeFromArgs(argc, argv);
}
