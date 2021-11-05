#include <rTEMPLATE.h>

#include <iostream>

using namespace rapio;

// Your group/project/area for all of YOUR code.
// Here we're part of wdssii or hmet or anc, etc.
using namespace wdssii;

/** A RAPIO Algorithm Template 
 * Real time Algorithm Parameters and I/O
 * http://github.com/retoomey/RAPIO
 *
 * Template for a new algorithm.
 *
 * @author Robert Toomey
 **/
void
TEMPLATE::declareOptions(RAPIOOptions& o)
{
   o.setDescription("TEMPLATE does something");
   o.setAuthors("Your name");

  // An optional string param...default is "Test" if not set by user
  // o.optional("T", "Test", "Test option flag");

  // An optional boolean param, since boolean defaults are always false.
  // o.boolean("x", "Turn x on and off for some reason.  Basically add a -x to turn on a boolean flag that is false by default");

  // A required parameter (algorithm won't run without it). 
  // Here there is no default since it's required, instead you can provide an example of the setting
  // o.require("Z", "method1", "Set this to anything, it's just an example");
}

/** RAPIOAlgorithms process options on start up */
void
TEMPLATE::processOptions(RAPIOOptions& o)
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
TEMPLATE::processNewData(rapio::RAPIOData& d)
{
  // See RAPIO/rexample/rSimpleAlg.cc for current experiments with
  // grib, etc. This template is meant to be shorter
  
#if 0
  // Example of dumping record information.  Datatype is lazy loaded
  // on .datatype call below
  std::string s = "Data received: ";
  auto sel      = d.record().getSelections();
  for (auto& s1:sel) {
    s = s + s1 + " ";
  }
  LogInfo(s << " from index " << d.matchedIndexNumber() << "\n");
#endif 

  // Look for _any_ data the system knows how to read
  //auto r = d.datatype<rapio::RadialSet>(); // Only RadialSet, for example
  auto r = d.datatype<rapio::DataType>();
  if (r != nullptr) {

    #if 0
    // Example of in-place changing a RadialSet
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
    #endif // if 0

    // Standard echo of data to output.  Note it's the same data out as in here
    LogInfo("--->Echoing " << r->getTypeName() << " product to output\n");
    writeOutputProduct(r->getTypeName(), r); // Typename will be replaced by -O filters
    LogInfo("--->Finished " << r->getTypeName() << " product to output\n");
  }
} // TEMPLATE::processNewData

void
TEMPLATE::processHeartbeat(const Time& n, const Time& p)
{
  LogInfo("Alg got a heartbeat...what do you want me to do?\n");
}

int
main(int argc, char * argv[])
{
  // Create your algorithm instance and run it.  Isn't this API better?
  TEMPLATE alg = TEMPLATE();

  // Run this thing standalone.
  alg.executeFromArgs(argc, argv);
}
