#include <rCopy.h>

#include <iostream>

using namespace rapio;

/** A simple copy tool */
void
Copy::declareOptions(RAPIOOptions& o)
{
  o.setDescription(
    "Copy is used to copy from one location to another using basic RAPIO filtering and format conversions if wanted.");
  o.optional("delaysecs", "-1", "A delay in seconds after processing a file.  Default is -1 which means no delay.");
}

/** RAPIOAlgorithms process options on start up */
void
Copy::processOptions(RAPIOOptions& o)
{
  myDelaySeconds = o.getFloat("delaysecs");
}

void
Copy::processNewData(rapio::RAPIOData& d)
{
  LogInfo("Data received: " << d.getDescription() << "\n");

  // Look for ANY data the system knows how to read
  auto data = d.datatype<rapio::DataType>();

  // And just copy it. All your options determine notification on/off, basic conversion, etc.
  if (data != nullptr) {
    std::map<std::string, std::string> myOverrides;
    writeOutputProduct(data->getTypeName(), data, myOverrides); // Typename will be replaced by -O filters
  }

  // Delay if wanted...
  if (myDelaySeconds != -1) {
    LogInfo("Delaying for " << myDelaySeconds << " seconds...\n");
    std::chrono::milliseconds dura((int) (myDelaySeconds * 1000.0)); // C++11 vs C sleep is a bit verbose
    std::this_thread::sleep_for(dura);
  }
} // Copy::processNewData

int
main(int argc, char * argv[])
{
  // Create your algorithm instance and run it.  Isn't this API better?
  Copy alg = Copy();

  // Run this thing standalone.
  alg.executeFromArgs(argc, argv);
}
