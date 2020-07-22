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

// Default always loaded datatype creation factories
#include "rIOXML.h"
#include "rIOJSON.h"


#include <iostream>
#include <string>

using namespace rapio;
using namespace std;

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
  o.require("o", "/data", "The output directory for generated products or data");
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
  o.addAdvancedHelp("n",
    "If blank, this is typically set to {OutputDir}/code_index.fam  If == 'disable' then all notification is turned off which could speed up processing an archive, for instance.");
}

void
RAPIOAlgorithm::processOutputParams(RAPIOOptions& o)
{
  // Add output products wanted and name translation filters
  std::string param = o.getString("O");
  addOutputProducts(param);

  // Gather output directory and link notifier.
  myNotifierPath = o.getString("n");
  myOutputDir    = o.getString("o");
}

void
RAPIOAlgorithm::processInputParams(RAPIOOptions& o)
{
  // Standard -i, -I -r handling of the input parameters,
  // most algorithms do it this way.
  // FIXME: ok do we disable this for things that use the product=this model,
  // such
  // as w2qcnndp.  Or make those use this and keep it all the same...which I'm
  // liking.
  // Have to think about it some more...
  std::string indexes  = o.getString("i");
  std::string products = o.getString("I");

  myReadMode = o.getString("r");
  myCronList = o.getString("sync");

  // Add the inputs we want to handle, using history setting...
  float history = o.getFloat("h");

  if (history < 0.001) {
    history = 15;
    LogSevere(
      "History -h value set too small, changing to " << history << "minutes.\n");
    myMaximumHistory = TimeDuration::Minutes(history);
  }
  addInputProducts(products);
  addIndexes(indexes, TimeDuration::Minutes(history));
}

TimeDuration
RAPIOAlgorithm::getMaximumHistory()
{
  return (myMaximumHistory);
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
  // FIXME: make configurable...more work here
  // Either allow alg options to tweak how this work, a config file
  // that defines it or something
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

  // -------------------------------------------------------------------
  // NETCDF READ/WRITE FUNCTIONALITY BUILT IN ALWAYS

  // Dynamic module registration.
  std::string create = "createRAPIOIO";

  std::string module = "librapionetcdf.so"; // does the name matter?
  Factory<IODataType>::introduceLazy("netcdf", module, create);
  Factory<IODataType>::introduceLazy("netcdf3", module, create);

  module = "librapiogrib.so";
  Factory<IODataType>::introduceLazy("grib", module, create);

  /*
   * // Force immediate loading, maybe good for testing
   * auto x = Factory<IODataType>::get("netcdf");
   * auto y = Factory<IODataType>::get("netcdf3");
   * auto z = Factory<IODataType>::get("grib");
   */

  // Everything should be registered, try initial start up
  Config::initialize();
  Unit::initialize(); // Think this could be a ConfigType.  It reads xml
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
    // Do all stock algorithms have this pretty much?  For the moment I'm
    // assuming yes...
    // FIXME: a method or maybe just part of declareOptions...
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
    o.processArgs(argc, argv); // Finally validate ALL arguments passed in.

    // 2.5 setup signals, etc..some stuff based upon arguments
    const int logFlush        = o.getInteger("flush");
    const std::string verbose = o.getString("verbose");
    Log::instance()->setInitialLogSettings(verbose, logFlush);

    initializeBaseline();

    // 3. Process options, inputs, outputs that were successfully parsed
    processOptions(o);      // Process generic options declared above
    processInputParams(o);  // Process stock input params declared above
    processOutputParams(o); // Process stock output params declared above

    // 4. Execute the algorithm (this will connect listeners, etc. )
    execute();
  } catch (std::string& e) {
    std::cerr << "Error: " << e << "\n";
  } catch (...) {
    std::cerr << "Uncaught exception (unknown type) \n";
  }
} // RAPIOAlgorithm::executeFromArgs

void
RAPIOAlgorithm::addInputProduct(const std::string& product)
{
  // inputs will be like this:
  // Reflectivity:00.50 VIL:00.50 etc.
  std::vector<std::string> pairs;
  Strings::splitWithoutEnds(product, ':', &pairs);

  // Add to our input info
  productInputInfo info;
  info.name = pairs[0];

  if (pairs.size() > 1) {
    info.subtype = pairs[1];
  }
  myProductInputInfo.push_back(info);
}

void
RAPIOAlgorithm::addInputProducts(const std::string& intype)
{
  // inputs will be like this:
  // Reflectivity:00.50 VIL:00.50 etc.
  std::vector<std::string> inputs;
  Strings::splitWithoutEnds(intype, ' ', &inputs);

  for (size_t i = 0; i < inputs.size(); ++i) {
    addInputProduct(inputs[i]);
  }
}

size_t
RAPIOAlgorithm::getInputNumber()
{
  return (myProductInputInfo.size());
}

void
RAPIOAlgorithm::addOutputProduct(const std::string& pattern)
{
  // Outputs will be like this:
  // Reflectivity:00.50 VIL:00.50 etc.
  // Reflectivity:00.50=MyRef1:00.50,  etc.
  // Reflectivity*=MyRef1*
  // I'm only supporting a single '*' match at moment...
  std::string fullproduct;
  std::string fullproductto;
  std::string productPattern   = "*"; // Match everything
  std::string subtypePattern   = "*";
  std::string toProductPattern = ""; // No conversion
  std::string toSubtypePattern = "";

  std::vector<std::string> pairs;
  Strings::splitWithoutEnds(pattern, '=', &pairs);

  if (pairs.size() > 1) { // X = Y form.  Remapping product/subtype names
    fullproduct   = pairs[0];
    fullproductto = pairs[1];

    pairs.clear();
    Strings::splitWithoutEnds(fullproductto, ':', &pairs);

    if (pairs.size() > 1) {
      toProductPattern = pairs[0];
      toSubtypePattern = pairs[1];
    } else {
      toProductPattern = pairs[0];
    }
  } else { // X form.  No remapping pattern...
    fullproduct = pairs[0];
  }

  // Split up the X full product pattern into pattern/subtype
  pairs.clear();
  Strings::splitWithoutEnds(fullproduct, ':', &pairs);

  if (pairs.size() > 1) {
    productPattern = pairs[0];
    subtypePattern = pairs[1];
  } else {
    productPattern = pairs[0];
    subtypePattern = "NOT FOUND";
  }

  //  LogInfo("Product pattern is " << productPattern << "\n");
  //  LogInfo("Subtype pattern is " << subtypePattern << "\n");
  //  LogInfo("Product2 pattern is " << toProductPattern << "\n");
  //  LogInfo("Subtype2 pattern is " << toSubtypePattern << "\n");

  // Make sure unique enough
  for (size_t i = 0; i < myProductOutputInfo.size(); i++) {
    if (productPattern == myProductOutputInfo[i].product) {
      LogInfo(
        "Already added output product with pattern '" << productPattern
                                                      << "', ignoring..\n");
      return;
    }
  }

  // Add the new output product declaration
  productOutputInfo info;
  info.product   = productPattern;
  info.subtype   = subtypePattern;
  info.toProduct = toProductPattern;
  info.toSubtype = toSubtypePattern;
  myProductOutputInfo.push_back(info);

  LogInfo("Added output product pattern with key '" << pattern << "'\n");
} // RAPIOAlgorithm::addOutputProduct

void
RAPIOAlgorithm::addOutputProducts(const std::string& intype)
{
  std::vector<std::string> outputs;
  Strings::splitWithoutEnds(intype, ' ', &outputs);

  for (size_t i = 0; i < outputs.size(); ++i) {
    addOutputProduct(outputs[i]);
  }
}

void
RAPIOAlgorithm::initOnStart(const IndexType& idx)
{
  // FIXME: method to set this on or off?
  // Accumulator doesn't use this..some algorithms do though

  /* initOnStart=false in algcreator XML file ...
   *   for (size_t i=0; i < myInputs.size(); ++i){
   *      std::vector<std::string> sel(1);
   *      sel.insert( sel.end(), myInputs[i].begin(), myInputs[i].end() );
   *      Record rec = idx.getMostCurrentRecord(sel);
   *      if ( rec.isValid() ) processInputField( rec );
   *   }
   */
}

void
RAPIOAlgorithm::addIndex(const std::string & index,
  const TimeDuration                       & maximumHistory)
{
  // We don't actually create an index here...we wait until execute.
  LogDebug("Adding source index:'" << index << "'\n");

  indexInputInfo info;

  // For the new xml=/home/code_index.xml protocol passing
  std::vector<std::string> pieces;

  // Macro ldm to the feedme binary by default for convenience
  if (index == "ldm") {
    info.protocol    = "exe";
    info.indexparams = "feedme%-f%TEXT";
  } else {
    // Split on = if able to get protocol, otherwise we'll try to guess
    Strings::split(index, '=', &pieces);
    const size_t aSize = pieces.size();
    if (aSize == 1) {
      info.protocol    = "";
      info.indexparams = pieces[0];
    } else if (aSize == 2) {
      info.protocol    = pieces[0];
      info.indexparams = pieces[1];
    } else {
      LogSevere("Format of passed index '" << index << "' is wrong, see help i\n");
      exit(1);
    }
  }
  // info.protocol       = "xml";
  // info.indexparams    = index;
  info.maximumHistory = maximumHistory;
  myIndexInputInfo.push_back(info);
} // RAPIOAlgorithm::addIndex

void
RAPIOAlgorithm::addIndexes(const std::string & param,
  const TimeDuration                         & maximumHistory)
{
  // Split indexes by spaces
  std::vector<std::string> pieces;
  Strings::splitWithoutEnds(param, ' ', &pieces);

  for (size_t p = 0; p < pieces.size(); ++p) {
    addIndex(pieces[p], maximumHistory);
  }
}

#if 0
namespace rapio {
class AlgorithmHeartbeat : public rapio::EventTimer {
public:

  AlgorithmHeartbeat(size_t milliseconds, size_t pulseSecs) : EventTimer(milliseconds, "Heartbeat"), myPulseSecs(
      pulseSecs)
  {
    // S, M, H, D
    // parse("*/5 * * *");
    parse("5 * * *");
  }

  std::vector<int> v;  // values
  std::vector<bool> m; // mode is slash or not

  bool
  parse(const std::string& cronlist)
  {
    // My baby cron format, except we have seconds as well
    // I don't think a realtime alg would want time settings
    // longer than week.  Can always expand/change format.
    // "Seconds, Mins, Hours, Day of week"
    // We need two time tweaks so far:
    // 1.  a 'lag' time to allow data to get there first.
    //     Basically allow say a 12:00 fire time to occur
    //     at 12:00 + lag.  This is for latency adjusting.
    // 2.  a 'min' time to keep restarting algorithms from
    //     immediately firing (because they have no memory)
    //     We could possibly use a marker file or something
    //     for this too...
    //
    // "*/5 * * *" -- every 5 seconds on mod 5
    // "0 * * * " -- on the 0 second.
    // "0 */2 * * " -- Every 2 min on the 0 second
    std::vector<std::string> fields;
    Strings::splitWithoutEnds(cronlist, ' ', &fields);
    while (fields.size() < 4) { fields.push_back("*"); }

    for (size_t i = 0; i < fields.size(); i++) {
      v.push_back(-1);
      m.push_back(false);

      std::vector<std::string> pairs;
      Strings::splitWithoutEnds(fields[i], '/', &pairs);

      if (pairs.size() == 2) { // Every mode
        if (pairs[0] == "*") {
          try{
            int value = std::stoi(pairs[1]);
            v[i] = value;
            m[i] = true; // from /
          } catch (...) {
            LogSevere("Bad / format for '" << fields[i] << "'\n");
            return false;
          }
        }
      } else {
        if (pairs[0] != "*") {
          try{
            int value = std::stoi(pairs[0]);
            v[i] = value;
          } catch (...) {
            LogSevere("Bad format for '" << fields[i] << "'\n");
            return false;
          }
        }
      }
    }
    return true;
  } // parse

  virtual void
  action() override
  {
    // Current time, or time of the 'system'
    // FIXME: with archive will need artifical times
    Time n = Time::CurrentTime();

    auto newSecond = v[0];

    // This is the */V every interval logic
    if (m[0] == true) {
      // LogSevere("/ logic: " << v[0] << "\n");
      // Match mod of value
      int syncSec = v[0];
      if (syncSec < 0) { syncSec = 0; }
      if (syncSec > 59) { syncSec = syncSec % 60; }
      auto second = n.getSecond();
      // auto newSecond = second-(second % syncSec);
      newSecond = second - (second % syncSec);
    } else {
      // LogSevere("NO / logic: " << v[0] << "\n");
      //    newSecond = v[0];  // match actual second value
    }

    // Calculate the sync time based on current
    Time p = Time(n.getYear(), n.getMonth(), n.getDay(),
        n.getHour(), n.getMinute(), newSecond, 0.0);

    // Oh if actual less wanted, don't fire.

    // Real time should be >= pulse wanted
    // and not equal to the last pulsetime
    // FIXME: minimum delay before first pulse to allow
    // alg time to gather data.
    if ((n >= p) && (p != myLastPulseTime)) {
      // LogSevere("Pulse ("<<p<<") at actual time: " << n << "\n");
      LogSevere(
        "PULSE:" << n.getHour() << ":" << n.getMinute() << ":" << n.getSecond() << " and " << p.getHour() << ":" << p.getMinute() << ":" << p.getSecond()
                 << "\n");
      myLastPulseTime = p;
    } else {
      // LogSevere("T:" << n.getHour() << ":"<<n.getMinute()<<":"<<n.getSecond() << " and " << p.getHour() << ":"<<p.getMinute() << ":" <<p.getSecond() << "\n");
    }
  } // action

  size_t myPulseSecs;
  Time myLastPulseTime;
};
}

#endif // if 0

namespace rapio {
class AlgRecordFilter : public RecordFilter
{
public:
  RAPIOAlgorithm * myAlg;

  void
  setAlg(RAPIOAlgorithm * m){ myAlg = m; }

  virtual bool
  wanted(const Record& rec) override
  {
    // FIXME: We also want to time filter in the time window,
    // expecially on startup reading..
    // Hack a time window for testing..
    //    TimeDuration dur = Time::CurrentTime() - rec.getTime();
    //    if (dur > TimeDuration::Seconds(180)){
    //     LogSevere("Record ignored due to age turn off hack...\n");
    //      return false;
    //    }
    int matchProduct = myAlg->matches(rec);

    return (matchProduct > -1);
  }
};
}

void
RAPIOAlgorithm::setUpRecordNotifier()
{
  // Set notification for new records..will all algorithms have this?
  myNotifier = RecordNotifier::getNotifier(myNotifierPath, myOutputDir, "fml");
  if (myNotifier == nullptr) {
    LogSevere("Notifier is not created.\n");
  } else {
    myNotifierPath = myNotifier->getURL().getPath();
  }
  LogInfo("Notifier path set to " << myNotifierPath << "\n");
}

void
RAPIOAlgorithm::setUpRecordFilter()
{
  std::shared_ptr<AlgRecordFilter> f = std::make_shared<AlgRecordFilter>();
  Record::theRecordFilter = f;
  f->setAlg(this);
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
  size_t count     = 0;
  size_t wanted    = myIndexInputInfo.size();
  bool daemon      = isDaemon();
  bool archive     = isArchive();
  std::string what = daemon ? "daemon" : "archive";
  if (daemon && archive) { what = "allrecords"; }

  for (size_t p = 0; p < wanted; ++p) {
    const indexInputInfo& i = myIndexInputInfo[p];
    std::shared_ptr<IndexType> in;

    bool success = false;
    in = IOIndex::createIndex(i.protocol, i.indexparams, i.maximumHistory);
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
  LogInfo("Output Directory is set to " << myOutputDir << "\n");

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

namespace {
/* Very simple end star support for the moment...this could probably be
 * coded more intelligently. */
bool
matchPattern(const std::string& pattern,
  const std::string           & tocheck,
  std::string                 & star)
{
  // "*"    "Velocity" YES "Velocity"
  // "Vel*" "Velocity" YES "ocity"
  // "Vel*" "Ref"      NO  ""
  // "Vel*" "Vel"      YES ""
  // ""     "Velocity" NO  ""
  // "Velocity" "VelocityOther" NO ""
  //
  bool match      = true;
  bool starFound  = false;
  const size_t pm = pattern.size();
  const size_t cm = tocheck.size();

  star = "";

  for (size_t i = 0; i < pm; i++) {
    if (pattern[i] == '*') { // Star found...
      starFound = true;
      match     = true;

      if (cm > i) { star = tocheck.substr(i); } break;
    } else if (pattern[i] != tocheck[i]) { // character mismatch
      match = false;
      break;
    }
  }

  // Ok if no star found should be _exact_ match
  // ie "Velocity" shouldn't match "VelocityOther" or "VelocityOther2"
  // but was good for "Velocity*" to match...
  if (!starFound && (cm > pm)) { // We checked up to length already
    match = false;
  }

  return (match);
} // matchPattern
}

// FIXME: move it all into the RecordFilter.
int
RAPIOAlgorithm::matches(const Record& rec)
{
  // sel is a set of strings like this: time datatype subtype
  // myType is a set of strings like  :      datatype subtype
  //                              or  :      datatype
  // i.e. we need to match sel[1] with myType[0]
  // and maybe sel[2] with myType[1]
  std::vector<std::string> sel = rec.getSelections();

  for (size_t i = 0; i < myProductInputInfo.size(); ++i) {
    const productInputInfo& p = myProductInputInfo[i];

    // If no subtype, just match the product pattern. For example, something
    // like
    // Vel* will match Velocity
    std::string star;
    bool matchProduct = false;
    bool matchSubtype = false;

    // -----------------------------------------------------------
    // Match product pattern if selections have it
    if (sel.size() > 1) {
      // Not sure we need this or why..it was in the old code though...
      if ((sel.back() != "vol") && (sel.back() != "all")) {
        matchProduct = matchPattern(p.name, sel[1], star);
      }
    }

    // -----------------------------------------------------------
    // Match subtype pattern if we have one (and product already matched)
    if (p.subtype == "") {
      matchSubtype = true; // empty is pretty much '*'
    } else {
      if (matchProduct && (sel.size() > 2)) {
        matchSubtype = matchPattern(p.subtype, sel[2], star);
      }
    }

    // Return on matched product/subtype
    if (matchProduct && matchSubtype) {
      //    matchedPattern = p.name;
      return (i);
    }
  }
  // matchedPattern = "";
  return (-1);
} // RAPIOAlgorithm::matches

void
RAPIOAlgorithm::handleRecordEvent(const Record& rec)
{
  if (rec.getSelections().back() == "EndDataset") {
    handleEndDatasetEvent();
  } else {
    // Give data to algorithm to process
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
    Log::flush();
    throw std::string("End of data set.");
  } else {
    // Realtime queue is empty, hey we're caught up...
  }
}

void
RAPIOAlgorithm::handleTimedEvent(const Time& n, const Time& p)
{
  // Dump heartbeat for now from the -sync option
  LogSevere(
    "Heartbeat actual:" << n.getHour() << ":" << n.getMinute() << ":" << n.getSecond() << " and for: " << p.getHour() << ":" << p.getMinute() << ":" << p.getSecond()
                        << "\n");
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

  for (size_t i = 0; i < myProductOutputInfo.size(); i++) {
    productOutputInfo& I = myProductOutputInfo[i];
    std::string p        = I.product;
    std::string p2       = I.toProduct;
    std::string star;

    if (matchPattern(p, key, star)) {
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

// Read the usage example below if you're trying to understand how to write
// outputs
// using this class
void
RAPIOAlgorithm::writeOutputProduct(const std::string& key,
  std::shared_ptr<DataType>                         outputData)
{
  std::string newProductName = "";

  if (productMatch(key, newProductName)) {
    LogInfo("Writing '" << key << "' as product name '" << newProductName
                        << "'\n");
    std::string typeName = outputData->getTypeName(); // Old one...
    outputData->setTypeName(newProductName);
    std::vector<Record> records;
    IODataType::write(outputData, myOutputDir, true, records, "netcdf");

    if (myNotifier != nullptr) {
      myNotifier->writeRecords(records);
    }
    outputData->setTypeName(typeName); // Restore old type name
                                       // not overriden.  Probably
                                       // doesn't matter.
  } else {
    LogInfo("DID NOT FIND a match for  product key " << key << "\n");
  }
}
