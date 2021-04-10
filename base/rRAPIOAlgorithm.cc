#include "rRAPIOAlgorithm.h"

#include "rRAPIOOptions.h"
#include "rError.h"
#include "rEventLoop.h"
#include "rEventTimer.h"
#include "rIOIndex.h"
#include "rIOWatcher.h"
#include "rTime.h"
#include "rStrings.h"
#include "rConfig.h"
#include "rUnit.h"
#include "rDataType.h"
#include "rIODataType.h"
#include "rIndexType.h"
#include "rProcessTimer.h"
#include "rRecordQueue.h"
#include "rAlgConfigFile.h"
#include "rFactory.h"
#include "rSignals.h"
#include "rHeartbeat.h"
#include "rDataTypeHistory.h"
#include "rStreamIndex.h"
#include "rRecordFilter.h"

// Default always loaded datatype creation factories
#include "rIOXML.h"
#include "rIOJSON.h"


#include <iostream>
#include <string>

using namespace rapio;
using namespace std;

TimeDuration RAPIOAlgorithm::myMaximumHistory;
Time RAPIOAlgorithm::myLastDataTime; // Defaults to epoch here
std::string RAPIOAlgorithm::myReadMode;

/** This listener class is connected to each index we create.
 * It does nothing but pass events onto the algorithm
 */

RAPIOAlgorithm::RAPIOAlgorithm()
{ }

void
RAPIOAlgorithm::declareInputParams(RAPIOOptions& o)
{
  // These are the standard generic 'input' parameters we support
  o.require("i",
    "/data/radar/KTLX/code_index.xml",
    "The input sources");
  o.addGroup("i", "I/O");
  o.addAdvancedHelp("i",
    "Use quotes and spaces to have multiple sources.  For example, -i \"//vmrms-sr20/KTLX /data/radar/code_index.xml\" means connect to two input sources, where the first is a web index, the second a xml index.");

  o.optional("I", "*", "The input type filter patterns");
  o.addGroup("I", "I/O");
  o.addAdvancedHelp("I",
    "Use quotes and spaces for multiple patterns.  For example, -I \"Ref* Vel*\" means match any product starting with Ref or Vel such as Ref10, Vel12. Or for example use \"Reflectivity\" to ingest stock Reflectivity from all -i sources.");

  // All stock algorithms support this
  // o.boolean("r", "Realtime mode for algorithm.");
  const std::string r = "r";
  o.optional(r, "old", "Read mode for algorithm.");
  o.addGroup(r, "TIME");
  o.addSuboption(r, "old", "Only process existing old records and stop.");
  o.addSuboption(r, "new", "Only process new incoming records. Same as -r");
  o.addSuboption(r, "", "Same as new, original realtime flag.");
  o.addSuboption(r, "all", "All old records, then wait for new.");

  o.optional("sync", "", "Sync data option. Cron format style algorithm heartbeat.");
  o.addGroup("sync", "TIME");
  o.addAdvancedHelp("sync",
    "In daemon/realtime sends heartbeat to algorithm.  Note if your algorithm lags you may miss heartbeat.  For example, you have a 1 min heartbeat but take 2 mins to calculate/write.  You will get the next heartbeat window.  The format is a 6 star second supported cronlist, such as '*/10 * * * * *' for every 10 seconds.");

  // All stock algorithms have a optional history setting.  We can overload this
  // in time
  // if more precision is needed.
  o.optional("h", "15", "History in minutes kept for inputs source(s).");
  o.addGroup("h", "TIME");
  o.addAdvancedHelp("h",
    "For indexes, this will be the global time in minutes for all indexes provided.");

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
  o.addAdvancedHelp("O",
    "With this, you specify products (datatypes) to output. For example, \"MyOutput1 MyOutput2\" means output only those two products.  \"MyOutput*\" means write anything starting with MyOutput.  Translating names is done by Key=Value.  For example \"MyOutput*=NeedThis*\" means change any product written out called MyOutput_min_qc to NeedThis_min_qc. The default is \"*\" which means any call to write(key) done by algorithm is matched and written to output.");
  o.optional("n",
    "",
    "The notifier for newly created files/records.");
  o.addGroup("n", "I/O");
}

void
RAPIOAlgorithm::addPostLoadedHelp(RAPIOOptions& o)
{
  // Dispatch advanced help to the mega static classes
  o.addAdvancedHelp("i", IOIndex::introduceHelp());
  o.addAdvancedHelp("n", RecordNotifier::introduceHelp());
  o.addAdvancedHelp("o", IODataType::introduceHelp());
}

void
RAPIOAlgorithm::processOutputParams(RAPIOOptions& o)
{
  // Add output products wanted and name translation filters
  const std::string param = o.getString("O");
  ConfigParamGroupO paramO;

  paramO.readString(param);

  // Gather notifier list and output directory
  myNotifierList = o.getString("n");
  ConfigParamGroupn paramn;
  paramn.readString(myNotifierList);

  // Gather output -o settings
  const std::string write = o.getString("o");
  ConfigParamGroupo paramo;
  paramo.readString(write);
} // RAPIOAlgorithm::processOutputParams

void
RAPIOAlgorithm::processInputParams(RAPIOOptions& o)
{
  // Add the inputs we want to handle, using history setting...
  float history = o.getFloat("h");

  // We create the history flag for others, but we're not
  // currently using it ourselves
  if (history < 0.001) {
    history = 15;
    LogSevere(
      "History -h value set too small, changing to " << history << "minutes.\n");
  }
  myMaximumHistory = TimeDuration::Minutes(history);

  // Standard -i, -I -r handling of the input parameters,
  // most algorithms do it this way.
  ConfigParamGroupi parami;
  parami.readString(o.getString("i"));

  ConfigParamGroupI paramI;
  paramI.readString(o.getString("I"));

  myReadMode = o.getString("r");
  myCronList = o.getString("sync");
}

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

void
RAPIOAlgorithm::initializeBaseParsers()
{
  // We have to introduce the 'raw' builder types
  // early since eveyrthing else will use them.
  // More advanced stuff like netcdf, etc will be
  // introduced later as system ramps up.
  std::shared_ptr<IOXML> xml = std::make_shared<IOXML>();
  Factory<IODataType>::introduce("xml", xml);
  Factory<IODataType>::introduce("W2ALGS", xml);

  std::shared_ptr<IOJSON> json = std::make_shared<IOJSON>();
  Factory<IODataType>::introduce("json", json);
}

void
RAPIOAlgorithm::initializeBaseline()
{
  // Typically algorithms are run in realtime for operations and
  // core dumping will fill up disks and cause more issues.
  // We're usually running/restarting from w2algrun...
  // FIXME: flags for this and logging
  // Signals::initialize(enableStackTrace, wantCoreDumps);

  Config::introduceSelf();

  // -------------------------------------------------------------------
  // IO watch support
  IOWatcher::introduceSelf();

  // -------------------------------------------------------------------
  // Index support
  IOIndex::introduceSelf();

  // -------------------------------------------------------------------
  // Notification support (Send xml records somewhere on processing)
  RecordNotifier::introduceSelf();

  // Everything should be registered, try initial start up
  if (!Config::initialize()) {
    LogSevere("Failed to initialize\n");
    exit(1);
  }
  Unit::initialize();

  /*
   * // Force immediate loading, maybe good for testing
   * // dynamic modules
   * auto x = Factory<IODataType>::get("netcdf");
   */
} // RAPIOAlgorithm::initializeBaseline

void
RAPIOAlgorithm::executeFromArgs(int argc, char * argv[])
{
  // Make sure initial logging setup
  // FIXME: chicken-egg problem.  We want configured logging from config,
  // but things print to log before config read, etc.
  // Need more work on ordering
  Log::instance();

  // Since this is called by a main function
  // wrap to catch any uncaught exception.
  try  {
    // Default header for an algorithm, subclasses should override
    RAPIOOptions o;
    o.setHeader(Constants::RAPIOHeader + "Algorithm");
    o.setDescription(
      "An Algorithm.  See example algorithm.  Call o.setHeader, o.setAuthors, o.setDescription in declareOptions to change.");
    o.setAuthors("Robert Toomey");

    // --------------------------------------------------------------------------------
    // We separate the stages here because I want to create modules later where
    // this order
    // of events may be critical:
    //
    // 1. Declare step
    declareFeatures();      // Declare extra features wanted...
    declareInputParams(o);  // Declare the input parameters used, default of
                            // i, I, l...
    declareOutputParams(o); // Declare the output parameters, default of o,
                            // O...
    declareOptions(o);      // Allow algorithm to declare wanted general
                            // arguments...

    // 2. Parse step
    // Algorithm special read/write of options support, processing args
    initializeBaseParsers(); // raw xml, json reading, etc.
    AlgConfigFile::introduceSelf();

    // Read/process arguments, don't react yet
    bool wantHelp = o.processArgs(argc, argv);

    // Initial loading of logging, settings/modules
    initializeBaseline();
    o.initToSettings();

    // Dump help and stop
    if (wantHelp) {
      // Dump advanced
      addPostLoadedHelp(o);
      o.finalizeArgs(wantHelp);
      exit(1);
    }

    // Allow options final validatation of ALL arguments passed in.
    bool success = o.finalizeArgs(wantHelp);
    if (!success) { exit(1); }

    // Change final logging now to arguments passed in.
    LogInfo("Algorithm initialized, executing " << OS::getProcessName() << "...\n");
    const std::string verbose = o.getString("verbose");
    Log::setSeverityString(verbose);
    Log::printCurrentLogSettings();

    // 3. Process options, inputs, outputs that were successfully parsed
    processOptions(o);      // Process generic options declared above
    processInputParams(o);  // Process stock input params declared above
    processOutputParams(o); // Process stock output params declared above

    // 4. Execute the algorithm (this will connect listeners, etc. )
    execute();
  } catch (const std::string& e) {
    std::cerr << "Error: " << e << "\n";
  } catch (...) {
    std::cerr << "Uncaught exception (unknown type) \n";
  }
} // RAPIOAlgorithm::executeFromArgs

void
RAPIOAlgorithm::setUpRecordNotifier()
{
  // Let record notifier factory create them, we will hold them though
  myNotifiers = RecordNotifier::createNotifiers();
}

void
RAPIOAlgorithm::setUpRecordFilter()
{
  std::shared_ptr<AlgRecordFilter> f = std::make_shared<AlgRecordFilter>(this);
  Record::theRecordFilter = f;
}

void
RAPIOAlgorithm::execute()
{
  // Add Heartbeat (if wanted)
  std::shared_ptr<Heartbeat> heart = nullptr;
  if (!myCronList.empty()) { // None required, don't make it
    LogInfo("Attempting to create heartbeat for " << myCronList << "\n");
    heart = std::make_shared<Heartbeat>(this, 1000);
    if (!heart->setCronList(myCronList)) {
      LogSevere("Bad format for -sync string, aborting\n");
      exit(1);
    }
  }

  // Create record queue and record filter/notifier
  std::shared_ptr<RecordQueue> q = std::make_shared<RecordQueue>(this);
  Record::theRecordQueue = q;

  setUpRecordNotifier();

  setUpRecordFilter();

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
  const size_t rSize = q->size();

  LogInfo(rSize << " initial records from " << wanted << " sources\n");
  LogDebug("Outputs:\n");
  const auto& writers = ConfigParamGroupo::getWriteOutputInfo();
  for (auto& w:writers) {
    LogDebug("   " << w.factory << "--> " << w.outputinfo << "\n");
  }

  // Time until end of program.  Make it dynamic memory instead of heap or
  // the compiler will kill it too early (too smartz)
  static std::shared_ptr<ProcessTimer> fulltime(
    new ProcessTimer("Algorithm total runtime"));

  // Add RecordQueue
  EventLoop::addTimer(q);

  if (heart != nullptr) {
    EventLoop::addTimer(heart);
  }

  // Do event loop
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
    // FIXME: 'Maybe' adjust this post DataType creation?
    myLastDataTime = rec.getTime();
    // FIXME: this back calls to inTimeWindow..couldn't it be cleaner?
    // FIXME: purge before or after adding newest?
    DataTypeHistory::purgeTimeWindow(myLastDataTime);
    RAPIOData d(rec);
    processNewData(d);
  }
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
    exit(0);
  } else {
    // Realtime queue is empty, hey we're caught up...
  }
}

void
RAPIOAlgorithm::handleTimedEvent(const Time& n, const Time& p)
{
  // Debug dump heartbeat for now from the -sync option
  LogDebug(
    "sync heartbeat:" << n.getHour() << ":" << n.getMinute() << ":" << n.getSecond() << " and for: " << p.getHour() << ":" << p.getMinute() << ":" << p.getSecond()
                      << "\n");
  processHeartbeat(n, p);
}

void
RAPIOAlgorithm::addFeature(const std::string& key)
{
  LogSevere(
    "Code programming error.  Request for unknown feature addition by key " << key
                                                                            << "\n");
  exit(1);
}

bool
RAPIOAlgorithm::productMatch(const std::string& key,
  std::string                                 & productName)
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
RAPIOAlgorithm::isProductWanted(const std::string& key)
{
  std::string newProductName = "";
  return (productMatch(key, newProductName));
}

void
RAPIOAlgorithm::writeOutputProduct(const std::string& key,
  std::shared_ptr<DataType> outputData,
  const std::map<std::string, std::string>& outputParams)
{
  std::string newProductName = "";

  if (productMatch(key, newProductName)) {
    LogInfo("Writing '" << key << "' as product name '" << newProductName
                        << "'\n");
    std::string typeName = outputData->getTypeName(); // Old one...
    outputData->setTypeName(newProductName);

    // Can call write multiple times for each output wanted.
    const auto& writers = ConfigParamGroupo::getWriteOutputInfo();
    for (auto& w:writers) {
      std::vector<Record> records;
      std::vector<std::string> files;
      IODataType::write(outputData, w.outputinfo, false, records, w.factory, outputParams);

      // Notify each notifier for each writer.
      // FIXME: Need a filter in -n to allow not calling?
      for (auto& n:myNotifiers) {
        // FIXME: For now assuming outputinfo is always a directory.
        // netcdf=/folder1 image=/folder2 gdal=/folder3
        // fml=/override
        // FIXME: Ok python is using a comma.  We'll have to refactor the API
        // a bit to clean this up.  Factories should control outputinfo --> output folder
        std::string outputfolder = w.outputinfo;
        std::vector<std::string> pieces;
        Strings::splitWithoutEnds(w.outputinfo, ',', &pieces);
        if (pieces.size() > 1) {
          outputfolder = pieces[1];
        }
        n->writeRecords(outputfolder, records);
      }
    }

    outputData->setTypeName(typeName); // Restore old type name
                                       // not overridden.  Probably
                                       // doesn't matter.
  } else {
    LogInfo("DID NOT FIND a match for product key " << key << "\n");
  }
} // RAPIOAlgorithm::writeOutputProduct
