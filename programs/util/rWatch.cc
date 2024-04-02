#include <rWatch.h>

#include <iostream>

using namespace rapio;

/** A simple monitor tool */
void
Watch::declareOptions(RAPIOOptions& o)
{
  o.setDescription("Watch is used to just monitor incoming RAPIO data.");
  // Make output option a /tmp optional, we don't need to force for monitoring
  o.setDefaultValue("o", "/tmp");
}

/** RAPIOAlgorithms process options on start up */
void
Watch::processOptions(RAPIOOptions& o)
{ }

void
Watch::processNewData(rapio::RAPIOData& d)
{
  // Look for ANY data the system knows how to read
  auto data = d.datatype<rapio::DataType>();

  // And just copy it. All your options determine notification on/off, basic conversion, etc.
  std::string valid    = "unknown";
  std::string TypeName = "unknown";

  if (data != nullptr) {
    valid    = "valid";
    TypeName = data->getTypeName();
  }

  static long counter = 0;

  counter++;
  LogInfo(counter << ": (" << valid << " " << TypeName << "): " << d.getDescription() << "\n");
} // Watch::processNewData

int
main(int argc, char * argv[])
{
  // Create your algorithm instance and run it.  Isn't this API better?
  Watch alg = Watch();

  // Run this thing standalone.
  alg.executeFromArgs(argc, argv);
}
