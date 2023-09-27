#include "rRAPIOAlgorithm.h"

#include "rRAPIOOptions.h"
#include "rRAPIOPlugin.h"
#include "rError.h"
#include "rEventLoop.h"
#include "rEventTimer.h"
#include "rTime.h"
#include "rStrings.h"
#include "rDataType.h"
#include "rIODataType.h"
#include "rIndexType.h"
#include "rProcessTimer.h"
#include "rRecordQueue.h"
#include "rFactory.h"
#include "rDataTypeHistory.h"
#include "rRecordFilter.h"
#include "rRecordNotifier.h"

// Baseline initialize
#include "rDataFilter.h"
#include "rIOWatcher.h"
#include "rIOIndex.h"
#include "rUnit.h"

#include <iostream>
#include <string>

using namespace rapio;
using namespace std;

TimeDuration RAPIOAlgorithm::myMaximumHistory;
Time RAPIOAlgorithm::myLastDataTime; // Defaults to epoch here

void
RAPIOAlgorithm::declareInputParams(RAPIOOptions& o)
{
  // These are the standard generic 'input' parameters we support
  o.require("i",
    "/data/radar/KTLX/code_index.xml",
    "The input sources");
  o.addGroup("i", "I/O");

  // The realtime option for reading archive, realtime, etc.
  const std::string r = "r";

  o.optional(r, "old", "Read mode for algorithm.");
  o.addGroup(r, "TIME");
  o.addSuboption(r, "old", "Only process existing old records and stop.");
  o.addSuboption(r, "new", "Only process new incoming records. Same as -r");
  o.addSuboption(r, "", "Same as new, original realtime flag.");
  o.addSuboption(r, "all", "All old records, then wait for new.");

  // All stock algorithms have a optional history setting.  We can overload this
  // in time
  // if more precision is needed.
  o.optional("h", "900", "History in seconds kept for inputs source(s).");
  o.addGroup("h", "TIME");

  // Feature to add all 'GRID' options for cutting lat lon grid data...
  // FIXME: Design Grid API that handles 2d and 3d and multiple grid options.
  // Design APi to autovalid grid option integrity
  // Toos many algs do this...
  // o.optional("grid", "", "Mega super grid option");
  // o.optional("t", "37 -100 20", "The top corner of grid");
  //  o.optional("b", "30.5 -93 1", "The bottom corner of grid");
  //  o.optional("s", "0.05 0.05 1", "The grid spacing");
  // --grid="37 -100 20,30.5 -93 1, 0.05 0.05 1"
} // RAPIOAlgorithm::declareInputParams

void
RAPIOAlgorithm::declareOutputParams(RAPIOOptions& o)
{
  // These are the standard generic 'output' parameters we support
  o.require("o", "/data", "The output writers/directory for generated products or data");
  o.addGroup("o", "I/O");
  o.optional("O",
    "*",
    "The output types patterns, controlling product names and writing");
  o.addGroup("O", "I/O");
  o.optional("postwrite",
    "",
    "Simple executable to call post file writing using %filename%.");
  o.addGroup("postwrite", "I/O");
  o.setHidden("postwrite");
}

void
RAPIOAlgorithm::addPostLoadedHelp(RAPIOOptions& o)
{
  // Dispatch advanced help to the mega static classes
  o.addAdvancedHelp("i", IOIndex::introduceHelp());

  // Datatype will try to load all dynamic modules, so we want to avoid that
  // unless help is requested
  if (o.wantAdvancedHelp("o")) {
    o.addAdvancedHelp("o", IODataType::introduceHelp());
  }

  // Static advanced on demand (lazy add)
  o.addAdvancedHelp("h",
    "For indexes, this will be the global time in minutes for all indexes provided.");
  o.addAdvancedHelp("O",
    "With this, you specify products (datatypes) to output. For example, \"MyOutput1 MyOutput2\" means output only those two products.  \"MyOutput*\" means write anything starting with MyOutput.  Translating names is done by Key=Value.  For example \"MyOutput*=NeedThis*\" means change any product written out called MyOutput_min_qc to NeedThis_min_qc. The default is \"*\" which means any call to write(key) done by algorithm is matched and written to output.");
  o.addAdvancedHelp("postwrite",
    "Allows you to run a command on a file output file. The 'ldm' command maps to 'pqinsert -v -f EXP %filename%', but any command in path can be ran using available macros.  Example: 'file %filename%' or 'ldm' or 'aws cp %filename'.");

  // Now let subclasses declare more things.
  // We do it this way to keep the algorithms from having to call superclass first
  declareAdvancedHelp(o);
}

void
RAPIOAlgorithm::processOutputParams(RAPIOOptions& o)
{
  // Add output products wanted and name translation filters
  const std::string param = o.getString("O");
  ConfigParamGroupO paramO;

  paramO.readString(param);

  // Gather output -o settings
  const std::string write = o.getString("o");
  ConfigParamGroupo paramo;

  paramo.readString(write);

  // Gather postwrite option
  myPostWrite = o.getString("postwrite");
} // RAPIOAlgorithm::processOutputParams

void
RAPIOAlgorithm::processInputParams(RAPIOOptions& o)
{
  // Add the inputs we want to handle, using history setting...
  float history = o.getFloat("h");

  // We create the history flag for others, but we're not
  // currently using it ourselves
  if (history < 0.001) {
    history = 900;
    LogSevere(
      "History -h value set too small, changing to " << history << "seconds.\n");
  }
  myMaximumHistory = TimeDuration::Seconds(history);

  // Standard -i, -I -r handling of the input parameters,
  // most algorithms do it this way.
  ConfigParamGroupi parami;

  parami.readString(o.getString("i"));

  myReadMode = o.getString("r");
} // RAPIOAlgorithm::processInputParams

TimeDuration
RAPIOAlgorithm::getMaximumHistory()
{
  return (myMaximumHistory);
}

bool
RAPIOAlgorithm::inTimeWindow(const Time& aTime)
{
  // For realtime use the now time?
  // Problem is we have all mode AND archive which won't match 'now' always
  // We'll try using the time of the latest received
  // product (which probably jitters a little).  I'm thinking this should be ok for most products)
  const TimeDuration window = myLastDataTime - aTime;

  // LogSevere("WINDOW FOR TIME: " << aTime << ", " << myLastDataTime << " ---> " << window << " MAX? " << myMaximumHistory << "\n");
  return (window <= myMaximumHistory);
}

bool
RAPIOAlgorithm::isDaemon()
{
  // If we are -r, -r=new or -r=new we are a daemon awaiting new records
  return ((myReadMode == "") || (myReadMode == "new") || (myReadMode == "all"));
}

bool
RAPIOAlgorithm::isArchive()
{
  // If we are missing -r, -r=old or -r=all we read archive
  return (myReadMode == "old") || (myReadMode == "all");
}

bool
RAPIOAlgorithm::isWebServer()
{
  for (auto p: myPlugins) {
    if (p->getName() == "web") {
      return p->isActive();
    }
  }
  return false;
}

void
RAPIOAlgorithm::initializeBaseline()
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
} // RAPIOAlgorithm::initializeBaseline

void
RAPIOAlgorithm::initializePlugins()
{
  // Stock plugins for Algorithm we setup.  Most user algorithms
  // will want these things, so we default to having them rather
  // than forcing subclass to call us to setup.
  // FIXME: We could not add them unless wanted in the particular
  // algorithm, might be better to be consistent though.
  PluginHeartbeat::declare(this, "sync"); // Heartbeat ability/options
  PluginWebserver::declare(this, "web");  // Webserver ability/options
  // I/O plugins
  PluginNotifier::declare(this, "n");     // Notification ability (fam, etc.)
  PluginRecordFilter::declare(this, "I"); // Record filter ability (ingest)
  // PluginProductFilter::declare(this, "O");  // Product filter ability (output)

  return RAPIOProgram::initializePlugins();
}

void
RAPIOAlgorithm::initializeOptions(RAPIOOptions& o)
{
  declareInputParams(o);  // Declare the input parameters used, default of
                          // i, I, l...
  declareOutputParams(o); // Declare the output parameters, default of o,

  return RAPIOProgram::initializeOptions(o);
}

void
RAPIOAlgorithm::finalizeOptions(RAPIOOptions& o)
{
  processInputParams(o);  // Process stock input params declared above
  processOutputParams(o); // Process stock output params declared above

  return RAPIOProgram::finalizeOptions(o);
}

void
RAPIOAlgorithm::setUpInitialIndexes()
{
  // Try to create an index for each source we want data from
  // and add sorted records to queue
  // FIXME: Archive should probably skip queue in case of giant archive cases,
  // otherwise we load a lot of memory.  We need better handling of this..
  // though for small archives we load all and sort, which is a nice ability
  size_t count      = 0;
  const auto& infos = ConfigParamGroupi::getIndexInputInfo();
  size_t wanted     = infos.size();
  bool daemon       = isDaemon();
  bool archive      = isArchive();
  std::string what  = daemon ? "daemon" : "archive";

  if (daemon && archive) { what = "allrecords"; }

  for (size_t p = 0; p < wanted; ++p) {
    const auto& i = infos[p];
    std::shared_ptr<IndexType> in;

    bool success = false;
    in = IOIndex::createIndex(i.protocol, i.indexparams, myMaximumHistory);
    myConnectedIndexes.push_back(in);

    if (in != nullptr) {
      in->setIndexLabel(p); // Mark in order
      success = in->initialRead(daemon, archive);
      if (success) {
        count++;
      }
    }

    std::string how = success ? "Successful" : "Failed";
    LogInfo(how << " " << what << " connection to '" << i.protocol << "-->" << i.indexparams << "'\n");
  }

  // Failed to connect to all needed sources...
  if (count != wanted) {
    LogSevere("Only connected to " << count << " of " << wanted
                                   << " data sources, ending.\n");
    exit(1);
  }
  const size_t rSize = Record::theRecordQueue->size();

  LogInfo(rSize << " initial records from " << wanted << " sources\n");
  LogDebug("Outputs:\n");
  const auto& writers = ConfigParamGroupo::getWriteOutputInfo();

  for (auto& w:writers) {
    LogDebug("   " << w.factory << "--> " << w.outputinfo << "\n");
  }
} // RAPIOAlgorithm::setUpInitialIndexes

void
RAPIOAlgorithm::setUpRecordQueue()
{
  // Create record queue and record filter/notifier
  std::shared_ptr<RecordQueue> q = std::make_shared<RecordQueue>(this);

  Record::theRecordQueue = q;
  EventLoop::addEventHandler(q);
}

void
RAPIOAlgorithm::execute()
{
  setUpRecordQueue();

  setUpInitialIndexes();

  // Plugins
  for (auto p: myPlugins) {
    p->execute(this);
  }

  // Time until end of program.  Make it dynamic memory instead of heap or
  // the compiler will kill it too early (too smartz)
  static std::shared_ptr<ProcessTimer> fulltime(
    new ProcessTimer("Algorithm total runtime"));

  EventLoop::doEventLoop();
} // RAPIOAlgorithm::execute

void
RAPIOAlgorithm::handleRecordEvent(const Record& rec)
{
  if (rec.getSelections().back() == "EndDataset") {
    handleEndDatasetEvent();
  } else {
    // Give data to algorithm to process
    // I've noticed some older indexes the record fractional
    // isn't equal to the loaded product (netcdf) fractional,
    // but unless we're doing indexed with nano data items probably
    // don't have to worry here.

    // I think realtime we purge off the clock, but 'archive' mode we purge
    // based on the 'latest' record in archive.  This requires archives to be
    // time sorted to work correctly, We'll try doing a 'max' as latest.
    if (isDaemon()) {
      myLastDataTime = Time::CurrentTime();
    } else {
      Time newTime = rec.getTime();
      if (newTime > myLastDataTime) {
        myLastDataTime = newTime;
      }
    }
    DataTypeHistory::purgeTimeWindow(myLastDataTime);
    RAPIOData d(rec);
    processNewData(d);
  }
}

void
RAPIOAlgorithm::processNewData(RAPIOData&)
{
  LogInfo("Received data, ignoring.\n");
}

void
RAPIOAlgorithm::processWebMessage(std::shared_ptr<WebMessage> w)
{
  LogInfo("Received web message, ignoring.\n");
  #if 0
  static int counter = 0;

  std::stringstream stream;

  stream << "<h1>Web request call number: " << ++counter << "</h1>";
  stream << "Path is: " << w->getPath() << "<br>";
  auto& map = w->getMap();

  stream << "There are " << map.size() << " fields.<br>";
  for (auto& a:map) {
    stream << "Field: " << a.first << " == " << a.second << "<br>";
  }
  w->setMessage(stream.str());
  #endif // if 0
}

void
RAPIOAlgorithm::handleEndDatasetEvent()
{
  if (!isDaemon()) {
    // Archive empty means end it all
    // FIXME: maybe just end event loop here, do a shutdown
    Log::setSeverity(Log::Severity::INFO);
    LogInfo(
      "End of archive data set, " << RecordQueue::poppedRecords << " of " << RecordQueue::pushedRecords
                                  << " processed.\n");
    Log::flush();
    EventLoop::exit(0);
  } else {
    // Realtime queue is empty, hey we're caught up...
  }
}

bool
RAPIOAlgorithm::isProductWanted(const std::string& key,
  std::string                                    & productName)
{
  bool found = false;

  std::string newProductName = "";

  const auto& outputs = ConfigParamGroupO::getProductOutputInfo();

  for (auto& I:outputs) {
    std::string p  = I.product;
    std::string p2 = I.toProduct;
    std::string star;

    if (Strings::matchPattern(p, key, star)) {
      found = true;

      if (p2 == "") { // No translation key, match only, use original key
        newProductName = key;
      } else {
        Strings::replace(p2, "*", star);
        newProductName = p2;
      }
      break;
    }
  }

  productName = newProductName;
  return (found);
}

bool
RAPIOAlgorithm::writeDirectOutput(const URL& path,
  std::shared_ptr<DataType>                outputData,
  std::map<std::string, std::string>       & outputParams)
{
  // return (IODataType::write(outputData, path.toString()));
  std::vector<Record> blackHole;

  outputParams["filepathmode"] = "direct";
  outputParams["postwrite"]    = myPostWrite;

  return IODataType::write(outputData, path.toString(), blackHole, "", outputParams); // Default write single file
}

void
RAPIOAlgorithm::writeOutputProduct(const std::string& key,
  std::shared_ptr<DataType>                         outputData,
  std::map<std::string, std::string>                & outputParams)
{
  outputParams["filepathmode"] = "datatype";
  outputParams["postwrite"]    = myPostWrite;

  std::string newProductName = "";

  if (isProductWanted(key, newProductName)) {
    // Original typeName, which may match key or not
    const std::string typeName = outputData->getTypeName();

    // Write DataType as given key, or as filtered to new product name by -O
    const bool changeProductName = (key != newProductName);
    if (changeProductName) {
      LogInfo("Writing '" << key << "' as product name '" << newProductName
                          << "'\n");
      outputData->setTypeName(newProductName);
    } else {
      outputData->setTypeName(key);
    }

    // Can call write multiple times for each output wanted.
    const auto& writers = ConfigParamGroupo::getWriteOutputInfo();
    for (auto& w:writers) {
      std::vector<Record> records;
      const bool success = IODataType::write(outputData, w.outputinfo, records, w.factory, outputParams);

      if (success) { // Only notify iff the file writes successfully
        if (!outputParams["showfilesize"].empty()) {
          LogInfo(outputParams["filename"] << " has filesize of " <<
            Strings::formatBytes(OS::getFileSize(outputParams["filename"])) << "\n");
        }

        // Get back the output folder for notifications
        // and notify each notifier for this writer.
        const std::string outputfolder = outputParams["outputfolder"];
        for (auto& n:PluginNotifier::theNotifiers) { // if any, use
          n->writeRecords(outputfolder, records);
        }
      } // Not gonna error..writers should be complaining
    }

    // Restore original typename, does matter since DataType might be reused.
    outputData->setTypeName(typeName);
  } else {
    LogInfo("DID NOT FIND a match for product key " << key << "\n");
  }
} // RAPIOAlgorithm::writeOutputProduct
