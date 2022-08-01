#include "rRAPIOProgram.h"

#include "rOS.h"
#include "rFactory.h"
#include "rConfig.h"

// Default always loaded datatype creation factories
#include "rIOXML.h"
#include "rIOJSON.h"
#include "rIOFile.h"

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

RAPIOOptions
RAPIOProgram::initializeOptions()
{
  RAPIOOptions o("Program");

  declareOptions(o);

  return o;
}

void
RAPIOProgram::finalizeOptions(RAPIOOptions& o)
{
  processOptions(o);
}

void
RAPIOProgram::executeFromArgs(int argc, char * argv[])
{
  // Since this is called by a main function
  // wrap to catch any uncaught exception.
  try  {
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
    // Option handling
    RAPIOOptions o      = initializeOptions();
    const bool wantHelp = o.processArgs(argc, argv);
    o.initToSettings(); // log colors

    // Dump help and stop
    if (wantHelp) {
      // Dump advanced
      addPostLoadedHelp(o);
      o.addPostLoadedHelp();
      o.finalizeArgs(wantHelp);
      exit(1);
    }
    const bool success = o.finalizeArgs(wantHelp);
    if (!success) { exit(1); }

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
RAPIOProgram::execute()
{
  LogInfo("This program doesn't do anything, override execute() method.\n");
}
