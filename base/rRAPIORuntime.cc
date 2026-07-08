#include "rRAPIORuntime.h"
#include "rNetwork.h"
#include "rLogger.h"
#include "rConfig.h"
#include "rIOXML.h"
#include "rIOJSON.h"
#include "rIOFile.h"
#include "rIOFakeDataType.h"
#include "rDataFilter.h"
#include "rIOWatcher.h"
#include "rIOIndex.h"
#include "rUnit.h"
#include "rFactory.h"
#include "rError.h"

using namespace rapio;

std::once_flag RAPIORuntime::ourInitFlag;

void
RAPIORuntime::initializeBaseParsers()
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

  // Introduce the builder for fake DataType generation
  std::shared_ptr<IOFakeDataType> fake = std::make_shared<IOFakeDataType>();
  Factory<IODataType>::introduce("fake", fake);

  fake->initialize();
}

void
RAPIORuntime::initializeBaseline()
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
RAPIORuntime::initialize()
{
  try {
    std::call_once(ourInitFlag, []() {
      // 1. Setup networking
      Network::setNetworkEngine("CURL");

      // 2. Setup dynamic logger
      Log::initialize();

      // 3. Register base factories
      initializeBaseParsers();
      Config::introduceSelf();
      initializeBaseline();

      // 4. Load configuration paths/files
      if (!Config::initialize()) {
        throw StartupException("Failed to initialize configuration framework (Config::initialize() failed).");
      }
    });
  } catch (const StartupException& e) {
    std::cerr << "\n[RAPIO STARTUP ERROR]: " << e.what() << "\n\n";
    exit(1);
  } catch (const std::exception& e) {
    std::cerr << "\n[UNHANDLED STD EXCEPTION DURING STARTUP]: " << e.what() << "\n\n";
    exit(1);
  }
}
