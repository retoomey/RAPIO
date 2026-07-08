#include "rRAPIOProgram.h"
#include "rRAPIOPlugin.h"

#include "rOS.h"
#include "rFactory.h"
#include "rConfig.h"
#include "rSignals.h"
#include "rIODataType.h"
#include "rRAPIORuntime.h"

// #include <memory>

using namespace rapio;
using namespace std;

void
RAPIOProgram::initializePlugins()
{
  // Any ability base to program called here
  // Currently for program we don't add anything extra at
  // the class level

  // Subclass extra
  declarePlugins();
}

void
RAPIOProgram::initializeOptions(RAPIOOptions& o)
{
  // Plugins get to go first in case an algorithm tries to use our standard flag
  for (auto p: myPlugins) {
    p->declareOptions(o);
  }

  declareOptions(o);
}

void
RAPIOProgram::finalizeOptions(RAPIOOptions& o)
{
  // Plugins get to go first in case an algorithm tries to use our standard flag
  for (auto p: myPlugins) {
    p->processOptions(o);
  }
  processOptions(o);
}

bool
RAPIOProgram::isWebServer(const std::string& key) const
{
  for (auto p: myPlugins) {
    if (p->getName() == key) {
      return p->isActive();
    }
  }
  return false;
}

bool
RAPIOProgram::writeDirectOutput(const URL& path,
  std::shared_ptr<DataType>              outputData,
  std::map<std::string, std::string>     & outputParams)
{
  std::vector<Record> blackHole;

  outputParams["filepathmode"] = "direct";

  return IODataType::write(outputData, path.toString(), blackHole, "", outputParams);
}

void
RAPIOProgram::executeFromArgs(int argc, char * argv[])
{
  // Since this is called by a main function
  // wrap to catch any uncaught exception.
  try  {
    // Fire up the environment (Loggers, Network, Factories, Config)
    // This is 100% thread-safe and will only ever run once per process.
    RAPIORuntime::initialize();

    // ------------------------------------------------------------
    // Declare plugins
    initializePlugins();

    // ------------------------------------------------------------
    // Option handling
    RAPIOOptions o(myDisplayClass);
    initializeOptions(o);
    const bool wantHelp = o.processArgs(argc, argv);
    o.initToSettings(); // log colors

    // Dump help and stop
    if (wantHelp) {
      // Dump advanced
      // Plugins get to go first in case an algorithm tries to use our standard flag
      for (auto p: myPlugins) {
        p->addPostLoadedHelp(o);
      }
      addPostLoadedHelp(o);
      o.addPostLoadedHelp();
      o.finalizeArgs(wantHelp);
      exit(0);
    }
    const bool success = o.finalizeArgs(wantHelp);
    if (!success) {
      throw StartupException("Option finalization failed due to unrecognized or invalid arguments.");
    }

    myMacroApplied = o.isMacroApplied();

    // Change final logging now to arguments passed in.

    // Final configured log settings off verbose flag
    const std::string verbose = o.getString("verbose");
    Log::setSeverityString(verbose); // Before logs

    // Feels silly for every run.  We have it in the help which
    // is probably 'enough' log spamming.
    // fLogInfo("{}", o.getHeader());
    fLogInfo("RAPIO Executing {}...", OS::getProcessName());
    fLogInfo("Build: ({})", OS::getBuildInfo());
    fLogInfo("{}", Signals::getCoreStatusString());
    Log::printCurrentLogSettings();

    // Final loading of validated options
    finalizeOptions(o);

    // Execute the algorithm
    execute();
  } catch (const StartupException& e) {
    // Expected user/configuration errors. Print cleanly to cerr.
    std::cerr << "\n[RAPIO STARTUP ERROR]: " << e.what() << "\n\n";
    exit(1);
  } catch (const RAPIOException& e) {
    // Other framework-level exceptions (for future expansion)
    std::cerr << "\n[RAPIO FATAL ERROR]: " << e.what() << "\n\n";
    exit(1);
  } catch (const std::exception& e) {
    // Standard library exceptions (std::bad_alloc, std::out_of_range, etc.)
    std::cerr << "\n[UNHANDLED STD EXCEPTION]: " << e.what() << "\n\n";
    exit(1);
  }
} // RAPIOProgram::executeFromArgs

void
RAPIOProgram::addPlugin(RAPIOPlugin * p)
{
  myPlugins.push_back(p);
}

void
RAPIOProgram::removePlugin(const std::string& name)
{
  for (size_t i = 0; i < myPlugins.size(); ++i) {
    if (myPlugins[i]->getName() == name) {
      RAPIOPlugin * removedItem = myPlugins[i];
      myPlugins.erase(myPlugins.begin() + i);
      delete removedItem;
    }
  }
}

void
RAPIOProgram::execute()
{
  // Plugins
  for (auto p: myPlugins) {
    p->execute(this);
  }
  fLogInfo("This program doesn't do anything, override execute() method.");
}
