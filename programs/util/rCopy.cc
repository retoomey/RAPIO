#include <rCopy.h>

#include <iostream>

using namespace rapio;

/** A simple copy tool */
void
Copy::declareOptions(RAPIOOptions& o)
{
  o.setDescription(
    "Copy is used to copy from one location to another using basic RAPIO filtering and format conversions if wanted.");
  o.optional("before", "-1", "A delay in seconds BEFORE processing a file.  Default is -1 which means no delay.");
  o.addGroup("before", "time");

  o.optional("after", "-1", "A delay in seconds AFTER processing a file.  Default is -1 which means no delay.");
  o.addGroup("after", "time");
  o.boolean("updatetime", "Change DataType time to current time when writing.");
  o.addGroup("updatetime", "time");
}

/** RAPIOAlgorithms process options on start up */
void
Copy::processOptions(RAPIOOptions& o)
{
  myBeforeDelaySeconds = o.getFloat("before");
  myAfterDelaySeconds  = o.getFloat("after");
  myUpdateTime         = o.getBoolean("updatetime");
}

void
Copy::processNewData(rapio::RAPIOData& d)
{
  fLogInfo("Data received: {}", d.getDescription());

  // Look for ANY data the system knows how to read
  auto data = d.datatype<rapio::DataType>();

  // And just copy it. All your options determine notification on/off, basic conversion, etc.
  if (data != nullptr) {
    // Before felay if wanted...
    if (myBeforeDelaySeconds != -1) {
      fLogInfo("Predelaying for {} seconds...", myBeforeDelaySeconds);
      std::chrono::milliseconds dura((int) (myBeforeDelaySeconds * 1000.0)); // C++11 vs C sleep is a bit verbose
      std::this_thread::sleep_for(dura);
    }

    std::map<std::string, std::string> myOverrides;
    // Force compression
    myOverrides["compression"] = "gz";
    if (myUpdateTime) {
      data->setTime(Time::CurrentTime());
    }
    writeOutputProduct(data->getTypeName(), data, myOverrides); // Typename will be replaced by -O filters

    // Delay if wanted...
    if (myAfterDelaySeconds != -1) {
      fLogInfo("Delaying for {} seconds...", myAfterDelaySeconds);
      std::chrono::milliseconds dura((int) (myAfterDelaySeconds * 1000.0)); // C++11 vs C sleep is a bit verbose
      std::this_thread::sleep_for(dura);
    }
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
