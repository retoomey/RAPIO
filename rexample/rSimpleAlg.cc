#include <rSimpleAlg.h>

using namespace rapio;
using namespace wdssii;

/** RAPIOAlgorithms handle declaring and processing general arguments */
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
  std::string s = "Data received: ";
  std::vector<std::string> sel = d.record().getSelections();
  for (size_t i = 0; i < sel.size(); i++) {
    s = s + sel[i] + " ";
  }
  LogInfo(s << " from index " << d.matchedIndexNumber() << "\n");

  std::shared_ptr<rapio::DataType> r = d.datatype<rapio::DataType>();
  if (r != nullptr) {
    LogInfo("--->Echoing " << r->getTypeName() << " product to output\n");
    writeOutputProduct(r->getTypeName(), *r); // Typename will be replaced by -O filters
  }
}
