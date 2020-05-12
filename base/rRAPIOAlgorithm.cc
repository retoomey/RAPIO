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

#include <iostream>
#include <map>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <algorithm>

#include <dlfcn.h>

// Datatype creation factories
// FIXME: Eventually want some sort of dynamic
// extension loading ability or something?
// #include "rIONetcdf.h"
// #include "rIOGrib.h"
#include "rIOXML.h"
#include "rIOJSON.h"

#include "rSignals.h"

using namespace rapio;
using namespace std;

/** This listener class is connected to each index we create.
 * It does nothing but pass events onto the algorithm
 */
namespace rapio {
class StockInputListener : public IndexListener {
public:

  StockInputListener(RAPIOAlgorithm * anAlg) : myAlg(anAlg){ }

  virtual ~StockInputListener()
  {
    myOn = false;
  }

  void
  setListening(bool flag)
  {
    myOn = flag;
  }

  void
  notifyNewRecordEvent(const Record& item) override
  {
    if (myOn) {
      myAlg->handleRecordEvent(item);
    }
  }

  void
  notifyEndDatasetEvent(const Record& item) override
  {
    if (myOn) {
      myAlg->handleEndDatasetEvent();
    }
  }

  // void notifyNewTimeStampEvent(IndexType *idx, const Record& item) {}

protected:

  bool myOn;
  RAPIOAlgorithm * myAlg;
};
}

RAPIOAlgorithm::RAPIOAlgorithm()
  : myInitOnStart(false),
  myHeartbeatSecs(0)
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
  o.boolean("r", "Realtime mode for algorithm.");
  o.addGroup("r", "TIME");

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
  bool realtime        = o.getBoolean("r");
  myRealtime = realtime;

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

  // Alpha: Dynamic module loading.
  // We'll make all the IO modules load from configuration, this
  // will allow plugins.  Start with netcdf for testing
  std::string create = "createRAPIOIO";
  std::string module = "librapionetcdf.so"; // does the name matter?
  std::shared_ptr<IODataType> dynamicNetcdf = OS::loadDynamic<IODataType>(module, create);
  if (dynamicNetcdf != nullptr) {
    dynamicNetcdf->initialize();
    // Read can read netcdf.  Can we read netcdf3?  humm
    Factory<IODataType>::introduce("netcdf", dynamicNetcdf);

    // Read can write netcdf/netcdf3
    Factory<IODataType>::introduce("netcdf3", dynamicNetcdf);
  }
  module = "librapiogrib.so"; // does the name matter?
  std::shared_ptr<IODataType> dynamicGrib = OS::loadDynamic<IODataType>(module, create);
  if (dynamicNetcdf != nullptr) {
    dynamicGrib->initialize();
    Factory<IODataType>::introduce("grib", dynamicGrib);
  }


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
  LogDebug("Adding source index url '" << index << "'\n");

  indexInputInfo info;

  // For the new xml=/home/code_index.xml protocol passing
  std::vector<std::string> pieces;
  Strings::split(index, '=', &pieces);
  const size_t aSize = pieces.size();
  if (aSize == 1) {
    info.protocol = "";
    info.indexURL = pieces[0];
  } else if (aSize == 2) {
    info.protocol = pieces[0];
    info.indexURL = pieces[1];
  } else {
    LogSevere("Format of passed index '" << index << "' is wrong, see help i\n");
    exit(1);
  }
  // info.protocol       = "xml";
  // info.indexURL       = index;
  info.maximumHistory = maximumHistory;
  myIndexInputInfo.push_back(info);
}

void
RAPIOAlgorithm::addIndexes(const std::string & param,
  const TimeDuration                         & maximumHistory)
{
  // Split the "-i" option up into multiple indexes
  std::vector<std::string> pieces;
  Strings::splitWithoutEnds(param, ' ', &pieces);

  for (size_t p = 0; p < pieces.size(); ++p) {
    addIndex(pieces[p], maximumHistory);
  }
}

void
RAPIOAlgorithm::setHeartbeat(size_t periodInSeconds)
{
  myHeartbeatSecs = periodInSeconds;
}

namespace rapio {
class AlgorithmHeartbeat : public rapio::EventTimer {
public:

  AlgorithmHeartbeat(size_t milliseconds) : EventTimer(milliseconds, "Heartbeat")
  { }

  virtual void
  action() override
  {
    /*
     *  LogSevere("---------------------->Heartbeat...\n");
     *  Time t = Time::CurrentTime();
     *  namespace C = std::chrono;
     *  namespace D = date;
     *
     *  // This is interesting...c+11 is just cooler and cooler
     *  // These are ALL smaller than out implementation
     *  auto tp = C::system_clock::now();
     *  auto dp = D::floor<date::days>(tp);
     *  auto ymd = D::year_month_day{dp};
     *  auto time = D::make_time(C::duration_cast<C::milliseconds>(tp-dp));
     *
     *  Date d = Date(t);
     *  LogSevere("Time is " << t << "\n");
     *  LogSevere("size of new one is " << sizeof(tp) << "\n");
     *  LogSevere("size of new date stuff is " << sizeof(dp) << ", " <<
     *     sizeof(ymd) << ", " << sizeof(time) << "\n");
     *  {
     *    using namespace date;
     *     std::cout << tp << "\n";
     *  }
     *  LogSevere("Calendar is " << d << "\n");
     *  LogSevere(ymd.year() << ", " << ymd.month() << ", " << ymd.day() << ", "
     *     <<
     *       "\n");
     *  LogSevere(time.hours().count() << "h, " <<
     *        time.minutes().count() << "m, " <<
     *        time.seconds().count() << "s, " <<
     *        time.subseconds().count() << "ms " <<
     *      "\n");
     */

    //  Log::flush();
  }
};
}

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
    myNotifierPath = myNotifier->getURL().path;
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
  // Timed test (FIXME: might need this ability later)
  myHeartbeatSecs = 1;

  if (myHeartbeatSecs > 0) { }
  std::shared_ptr<AlgorithmHeartbeat> flusher = std::make_shared<AlgorithmHeartbeat>(5000);
  EventLoop::addTimer(flusher);

  LogInfo("Output Directory is set to " << myOutputDir << "\n");

  setUpRecordNotifier();

  // Create a single listener to connect to all required indexes.
  std::vector<std::shared_ptr<IndexListener> > llist;
  std::shared_ptr<StockInputListener> l = std::make_shared<StockInputListener>(this);
  l->setListening(false);
  llist.push_back(l);

  setUpRecordFilter();

  // Actually create all the indexes and link listener to each one.
  size_t count  = 0;
  size_t rcount = 0;
  size_t wanted = myIndexInputInfo.size();

  // Create record queue
  std::shared_ptr<RecordQueue> q = std::make_shared<RecordQueue>(this, myRealtime);
  Record::theRecordQueue = q;

  // Try to create an index for each source we want data from
  // and add sorted records to queue
  for (size_t p = 0; p < wanted; ++p) {
    const indexInputInfo& i = myIndexInputInfo[p];
    std::shared_ptr<IndexType> in;

    bool success = false;
    in = IOIndex::createIndex(i.protocol, i.indexURL, llist, i.maximumHistory);
    if (in != nullptr) {
      myConnectedIndexes.push_back(in);        // Since we just create object now, should
                                               // be good
      myConnectedIndexes[p]->setIndexLabel(p); // Mark in order
      success = myConnectedIndexes[p]->initialRead(myRealtime);
    }
    if (success) {
      count++;

      if (myRealtime) {
        rcount++;
      }
    }

    std::string how  = success ? "Successful" : "Failed";
    std::string what = myRealtime ? "realtime" : "archive";
    LogInfo(how << " " << what << " connection to '" << i.indexURL << "'\n");
  }

  // Failed to connect to all needed sources...
  if (count != wanted) {
    LogSevere("Only connected to " << count << " of " << wanted
                                   << " data sources, ending.\n");
    exit(1);
  }
  // const size_t rSize = initialRecords.size();
  const size_t rSize = q->size();

  LogInfo(rSize << " initial records from " << wanted << " sources\n");

  // Time until end of program.  Make it dynamic memory instead of heap or
  // the compiler will kill it too early (too smartz)
  static std::shared_ptr<ProcessTimer> fulltime(
    new ProcessTimer("Algorithm total runtime"));

  q->setConnectedIndexes(myConnectedIndexes);
  EventLoop::addTimer(q);
  l->setListening(true); // FIXME: shouldn't matter anymore

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
  // Record filter already took out any unwanted records
  // at this point.
  RAPIOData d(rec, rec.getIndexNumber());

  // Give data to algorithm to process
  processNewData(d);
}

void
RAPIOAlgorithm::handleEndDatasetEvent()
{
  // Throw anonymous temporary...
  Log::flush();
  throw std::string("End of data set.");
}

void
RAPIOAlgorithm::handleTimedEvent()
{
  LogSevere("Time event called in algorithm!\n");
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
    // IODataType::writeData(outputData, myOutputDir, records);
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
