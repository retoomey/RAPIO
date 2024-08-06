#include "rRAPIOProgram.h"
#include "rRAPIOPlugin.h"

#include "rOS.h"
#include "rFactory.h"
#include "rConfig.h"

// Default always loaded datatype creation factories
#include "rNetwork.h"
#include "rIOXML.h"
#include "rIOJSON.h"
#include "rIOFile.h"

// Baseline initialize
// A lot of these link to plugins...so do we lazy init
// into the plugin model
#include "rDataFilter.h"
#include "rIOWatcher.h"
#include "rIOIndex.h"
#include "rUnit.h"

#include <memory>

using namespace rapio;
using namespace std;

void
RAPIOProgram::initializeBaseParsers()
{
  // We have to introduce the 'raw' builder types
  // early since eveyrthing else will use them.
  // More advanced stuff like netcdf, etc will be
  // introduced later as system ramps up.
  // Pretty much 'any' program will need to read configurations
  // and single files so we'll always add this
  // ability
  std::shared_ptr<IOXML> xml = std::make_shared<IOXML>();
  Factory<IODataType>::introduce("xml", xml);
  Factory<IODataType>::introduce("w2algs", xml);

  xml->initialize();

  std::shared_ptr<IOJSON> json = std::make_shared<IOJSON>();
  Factory<IODataType>::introduce("json", json);

  json->initialize();

  std::shared_ptr<IOFile> file = std::make_shared<IOFile>();
  Factory<IODataType>::introduce("file", file);

  file->initialize();
}

void
RAPIOProgram::initializeBaseline()
{
  // Many of these modules/abilities register with config to load
  // settings

  // -------------------------------------------------------------------
  // Data filter support (compression endings)
  DataFilter::introduceSelf();

  // -------------------------------------------------------------------
  // IO watch support
  IOWatcher::introduceSelf();

  // -------------------------------------------------------------------
  // Index support
  IOIndex::introduceSelf();

  // -------------------------------------------------------------------
  // Unit conversion support
  Unit::initialize();
}

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
    // Default engine for URL pulling, etc.
    // FIXME: I'm trying to eventually get rid of CURL
    // here for one less dependency.
    Network::setNetworkEngine("CURL");
    // Network::setNetworkEngine("BOOST");

    // ------------------------------------------------------------
    // Initial logging ability (default configured)
    Log::instance();

    // Raw xml, json, single file ability reading, etc.
    initializeBaseParsers();

    // ------------------------------------------------------------
    // Load configurations
    Config::introduceSelf();

    // Initial loading of config to each module, parser, etc.
    initializeBaseline();

    // Everything should be registered, try initial start up
    if (!Config::initialize()) {
      LogSevere("Failed to initialize.\n");
      exit(1);
    }

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
    if (!success) { exit(1); }

    myMacroApplied = o.isMacroApplied();

    // Change final logging now to arguments passed in.
    LogInfo("Executing " << OS::getProcessName() << "...\n");

    // Final configured log settings off verbose flag
    const std::string verbose = o.getString("verbose");
    Log::setSeverityString(verbose);
    Log::printCurrentLogSettings();

    // Final loading of validated options
    finalizeOptions(o);

    // Execute the algorithm
    execute();
  } catch (const std::exception& e) {
    std::cerr << "Exception during startup: " << e.what() << "\n";
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
  LogInfo("This program doesn't do anything, override execute() method.\n");
}
