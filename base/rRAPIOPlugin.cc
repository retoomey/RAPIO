#include "rRAPIOPlugin.h"

#include <rRAPIOAlgorithm.h>

#include <rError.h>
#include <rEventLoop.h>
#include <rRAPIOOptions.h>
#include <rStrings.h>
#include <rIODataType.h>
#include <rColorTerm.h>

// FIXME: Eventually break up the plugins into files?
#include <rHeartbeat.h>
#include <rWebServer.h>
#include <rConfigParamGroup.h>
#include <rRecordFilter.h>
#include <rRecordNotifier.h>
#include <rVolumeValueResolver.h>
#include <rIOIndex.h>
#include <rRecordQueue.h>

using namespace rapio;
using namespace std;

std::vector<std::shared_ptr<RecordNotifierType> > PluginNotifier::theNotifiers;

bool
PluginHeartbeat::declare(RAPIOProgram * owner, const std::string& name)
{
  owner->addPlugin(new PluginHeartbeat(name));
  return true;
}

void
PluginHeartbeat::declareOptions(RAPIOOptions& o)
{
  o.optional(myName, "", "Sync data option. Cron format style algorithm heartbeat.");
  o.addGroup(myName, "TIME");
}

void
PluginHeartbeat::addPostLoadedHelp(RAPIOOptions& o)
{
  o.addAdvancedHelp(myName,
    "Sends heartbeat to program.  Note if your program lags you may miss heartbeat.  For example, you have a 1 min heartbeat but take 2 mins to calculate/write.  You will get the next heartbeat window.  The format is a 6 star second supported cronlist, such as '*/10 * * * * *' for every 10 seconds.");
}

void
PluginHeartbeat::processOptions(RAPIOOptions& o)
{
  myCronList = o.getString(myName);
}

void
PluginHeartbeat::execute(RAPIOProgram * caller)
{
  // Add Heartbeat (if wanted)
  std::shared_ptr<Heartbeat> heart = nullptr;

  if (!myCronList.empty()) { // None required, don't make it
    LogInfo("Attempting to create a heartbeat for " << myCronList << "\n");

    heart = std::make_shared<Heartbeat>((RAPIOAlgorithm *) (caller), 1000);
    if (!heart->setCronList(myCronList)) {
      LogSevere("Bad format for -" << myName << " string, aborting\n");
      exit(1);
    }
    EventLoop::addEventHandler(heart);
    myActive = true;
  }
}

bool
PluginWebserver::declare(RAPIOProgram * owner, const std::string& name)
{
  owner->addPlugin(new PluginWebserver(name));
  return true;
}

void
PluginWebserver::declareOptions(RAPIOOptions& o)
{
  o.optional(myName,
    "off",
    "Web server ability for REST pull algorithms. Use a number to assign a port.");
  o.setHidden(myName);
  o.addGroup("web", "I/O");
}

void
PluginWebserver::addPostLoadedHelp(RAPIOOptions& o)
{
  o.addAdvancedHelp(myName,
    "Allows you to run the algorithm as a web server.  This will call processWebMessage within your algorithm.  -web=8080 runs your server on http://localhost:8080.");
}

void
PluginWebserver::processOptions(RAPIOOptions& o)
{
  myWebServerMode = o.getString(myName);
}

void
PluginWebserver::execute(RAPIOProgram * caller)
{
  const bool wantWeb = (myWebServerMode != "off");

  myActive = wantWeb;
  if (wantWeb) {
    WebServer::startWebServer(myWebServerMode, caller);
  }
}

bool
PluginNotifier::declare(RAPIOProgram * owner, const std::string& name)
{
  static bool once = true;

  if (once) {
    // Humm some plugins might require dynamic loading so
    // this might change.  Used to do this in initializeBaseline
    // now we're lazy loading.  Upside is save some memory
    // if no notifications wanted
    RecordNotifier::introduceSelf();
    owner->addPlugin(new PluginNotifier(name));
    once = false;
  } else {
    LogSevere("Code error, can only declare notifiers once\n");
    exit(1);
  }
  return true;
}

void
PluginNotifier::declareOptions(RAPIOOptions& o)
{
  o.optional(myName,
    "",
    "The notifier for newly created files/records.");
  o.addGroup(myName, "I/O");
}

void
PluginNotifier::addPostLoadedHelp(RAPIOOptions& o)
{
  // FIXME: plug could just do it
  o.addAdvancedHelp(myName, RecordNotifier::introduceHelp());
}

void
PluginNotifier::processOptions(RAPIOOptions& o)
{
  // Gather notifier list and output directory
  myNotifierList = o.getString("n");
  // FIXME: might move to plugin? Seems overkill now
  ConfigParamGroupn paramn;

  paramn.readString(myNotifierList);
}

void
PluginNotifier::execute(RAPIOProgram * caller)
{
  // Let record notifier factory create them, we will hold them though
  theNotifiers = RecordNotifier::createNotifiers();
  myActive     = true;
}

bool
PluginIngestor::declare(RAPIOProgram * owner, const std::string& name)
{
  static bool once = true; // FIXME: could we make it multi though?

  if (once) {
    owner->addPlugin(new PluginIngestor(name));
    once = false;
  } else {
    LogSevere("Code error, can only declare ingestor once\n");
    exit(1);
  }
  return true;
}

void
PluginIngestor::declareOptions(RAPIOOptions& o)
{
  o.require(myName,
    "/data/radar/KTLX/code_index.xml",
    "The input sources");
  o.addGroup(myName, "I/O");
}

void
PluginIngestor::addPostLoadedHelp(RAPIOOptions& o)
{
  // The 'i' plugin relies on the Index::initialize called
  o.addAdvancedHelp(myName, IOIndex::introduceHelp());
}

void
PluginIngestor::processOptions(RAPIOOptions& o)
{
  ConfigParamGroupi parami; // Thinking about deprecating these

  parami.readString(o.getString(myName));
}

void
PluginIngestor::execute(RAPIOProgram * callerin)
{
  // The old setUpInitialIndexes from algorithm
  // belongs here. We only need indexes iff we have the 'i' option

  // Set up record queue for indexes to write to.  This couples the queue
  // with the -i option, I guess it could be made a separate plugin if needed later.
  // Create record queue and record filter/notifier
  RAPIOAlgorithm * caller = (RAPIOAlgorithm *) (callerin); // FIXME: bad design

  std::shared_ptr<RecordQueue> q = std::make_shared<RecordQueue>(caller);

  Record::theRecordQueue = q;
  EventLoop::addEventHandler(q);

  // Try to create an index for each source we want data from
  // and add sorted records to queue
  // FIXME: Archive should probably skip queue in case of giant archive cases,
  // otherwise we load a lot of memory.  We need better handling of this..
  // though for small archives we load all and sort, which is a nice ability
  size_t count         = 0;
  const auto& infos    = ConfigParamGroupi::getIndexInputInfo();
  size_t wanted        = infos.size();
  bool daemon          = caller->isDaemon();
  bool archive         = caller->isArchive();
  TimeDuration history = caller->getMaximumHistory();
  std::string what     = daemon ? "daemon" : "archive";

  if (daemon && archive) { what = "allrecords"; }

  for (size_t p = 0; p < wanted; ++p) {
    const auto& i = infos[p];
    std::shared_ptr<IndexType> in;

    bool success = false;
    in = IOIndex::createIndex(i.protocol, i.indexparams, history);
    myConnectedIndexes.push_back(in); // FIXME: Not sure we need this

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

  myActive = true;
} // PluginIngestor::execute

bool
PluginRecordFilter::declare(RAPIOProgram * owner, const std::string& name)
{
  static bool once = true;

  if (once) {
    // FIXME: ability to subclass/replace?
    std::shared_ptr<AlgRecordFilter> f = std::make_shared<AlgRecordFilter>();
    Record::theRecordFilter = f;
    owner->addPlugin(new PluginRecordFilter(name));
    once = false;
  } else {
    LogSevere("Code error, can only declare record filter once\n");
    exit(1);
  }
  return true;
}

void
PluginRecordFilter::declareOptions(RAPIOOptions& o)
{
  o.optional(myName, "*", "The input type filter patterns");
  o.addGroup(myName, "I/O");
}

void
PluginRecordFilter::addPostLoadedHelp(RAPIOOptions& o)
{
  o.addAdvancedHelp(myName,
    "Use quotes and spaces for multiple patterns.  For example, -I \"Ref* Vel*\" means match any product starting with Ref or Vel such as Ref10, Vel12. Or for example use \"Reflectivity\" to ingest stock Reflectivity from all -i sources.");
}

void
PluginRecordFilter::processOptions(RAPIOOptions& o)
{
  ConfigParamGroupI paramI;

  paramI.readString(o.getString(myName));
}

/** Execute/run the plugin */
void
PluginRecordFilter::execute(RAPIOProgram * caller)
{
  // Possibly could create here instead of in declare?
  // doesn't matter much in this case, maybe later it will
  // std::shared_ptr<AlgRecordFilter> f = std::make_shared<AlgRecordFilter>();
  // Record::theRecordFilter = f;
  myActive = true;
}

bool
PluginProductOutput::declare(RAPIOProgram * owner, const std::string& name)
{
  owner->addPlugin(new PluginProductOutput(name));
  return true;
}

void
PluginProductOutput::declareOptions(RAPIOOptions& o)
{
  // These are the standard generic 'output' parameters we support
  o.require("o", "/data", "The output writers/directory for generated products or data");
  o.addGroup("o", "I/O");
}

void
PluginProductOutput::addPostLoadedHelp(RAPIOOptions& o)
{
  // Datatype will try to load all dynamic modules, so we want to avoid that
  // unless help is requested
  if (o.wantAdvancedHelp("o")) {
    o.addAdvancedHelp("o", IODataType::introduceHelp());
  }
}

void
PluginProductOutput::processOptions(RAPIOOptions& o)
{
  ConfigParamGroupo paramO;

  paramO.readString(o.getString(myName));
}

bool
PluginProductOutputFilter::declare(RAPIOProgram * owner, const std::string& name)
{
  owner->addPlugin(new PluginProductOutputFilter(name));
  return true;
}

void
PluginProductOutputFilter::declareOptions(RAPIOOptions& o)
{
  o.optional(myName, "*", "The output products, controlling on/off and name remapping");
  o.addGroup(myName, "I/O");
}

void
PluginProductOutputFilter::addPostLoadedHelp(RAPIOOptions& o)
{
  std::string help =
    "Allows on/off and name change of output datatypes. For example, \"*\" means all products. Translating names is done by Key=Value. Keys are used to reference a particular product being written, while values can be name translations for multiple instances of an algorithm to avoid product write clashing.  For example, \"Reflectivity=Ref2QC Velocity=Vel1QC\"\n";

  // Add list of registered static products
  if (myKeys.size() > 0) {
    help += ColorTerm::fBlue + "Registered product output keys:" + ColorTerm::fNormal + "\n";
    for (size_t i = 0; i < myKeys.size(); ++i) {
      help += " " + ColorTerm::fRed + myKeys[i] + ColorTerm::fNormal + " : " + myKeyHelp[i] + "\n";
    }
  }
  o.addAdvancedHelp(myName, help);
}

void
PluginProductOutputFilter::processOptions(RAPIOOptions& o)
{
  ConfigParamGroupO paramO;

  paramO.readString(o.getString(myName));
}

bool
PluginProductOutputFilter::isProductWanted(const std::string& key)
{
  const auto& outputs = ConfigParamGroupO::getProductOutputInfo();

  for (auto& I:outputs) {
    std::string p = I.product;
    std::string star;

    if (Strings::matchPattern(p, key, star)) {
      return true;
    }
  }

  return false;
}

std::string
PluginProductOutputFilter::resolveProductName(const std::string& key,
  const std::string                                            & productName)
{
  std::string newProductName = productName;

  const auto& outputs = ConfigParamGroupO::getProductOutputInfo();

  for (auto& I:outputs) {
    // If key is found...
    std::string star;
    if (Strings::matchPattern(I.product, key, star)) {
      // ..translate the productName if there's a "Key=translation"
      std::string p2 = I.toProduct;
      if (p2 != "") {
        Strings::replace(p2, "*", star);
        newProductName = p2;
      }
      break;
    }
  }

  return newProductName;
}

bool
PluginVolumeValueResolver::declare(RAPIOProgram * owner, const std::string& name)
{
  VolumeValueResolver::introduceSelf(); // static
  owner->addPlugin(new PluginVolumeValueResolver(name));
  return true;
}

void
PluginVolumeValueResolver::declareOptions(RAPIOOptions& o)
{
  o.optional(myName, "lak",
    "Value Resolver Algorithm, such as 'lak', or your own. Params follow: lak,params.");
  VolumeValueResolver::introduceSuboptions(myName, o);
}

void
PluginVolumeValueResolver::addPostLoadedHelp(RAPIOOptions& o)
{
  o.addAdvancedHelp(myName, VolumeValueResolver::introduceHelp());
}

void
PluginVolumeValueResolver::processOptions(RAPIOOptions& o)
{
  myResolverAlg = o.getString(myName);
}

void
PluginVolumeValueResolver::execute(RAPIOProgram * caller)
{
  // -------------------------------------------------------------
  // VolumeValueResolver creation
  // Check for resolver existance at execute and fail if invalid
  // FIXME: Could reduce code here and handle param.
  myResolver = VolumeValueResolver::createFromCommandLineOption(myResolverAlg);

  // Stubbornly refuse to run if Volume Value Resolver requested by name and not found or failed
  if (myResolver == nullptr) {
    LogSevere("Volume Value Resolver '" << myResolverAlg << "' requested, but failed to find and/or initialize.\n");
    exit(1);
  } else {
    LogInfo("Using Volume Value Resolver: '" << myResolverAlg << "'\n");
  }
}

std::shared_ptr<VolumeValueResolver>
PluginVolumeValueResolver::getVolumeValueResolver()
{
  return myResolver;
}

bool
PluginTerrainBlockage::declare(RAPIOProgram * owner, const std::string& name)
{
  TerrainBlockage::introduceSelf(); // static
  owner->addPlugin(new PluginTerrainBlockage(name));
  return true;
}

void
PluginTerrainBlockage::declareOptions(RAPIOOptions& o)
{
  // Terrain blockage plugin
  // FIXME: Lak is introduced right...so where is this declared generically?
  o.optional(myName, "lak", // default is lak eh
    "Terrain blockage algorithm. Params follow: lak,/DEMS.  Most take root folder of DEMS.");
  TerrainBlockage::introduceSuboptions("terrain", o);
  o.addSuboption("terrain", "", "Don't apply any terrain algorithm.");
}

void
PluginTerrainBlockage::addPostLoadedHelp(RAPIOOptions& o)
{
  o.addAdvancedHelp(myName, TerrainBlockage::introduceHelp());
}

void
PluginTerrainBlockage::processOptions(RAPIOOptions& o)
{
  myTerrainAlg = o.getString(myName);
}

void
PluginTerrainBlockage::execute(RAPIOProgram * caller)
{
  // Nothing, a unique terrain is lazy requested with extra params
}

std::shared_ptr<TerrainBlockage>
PluginTerrainBlockage::getNewTerrainBlockage(
  const LLH         & radarLocation,
  const LengthKMs   & radarRangeKMs,
  const std::string & radarName,
  LengthKMs         minTerrainKMs, // do we need these?
  AngleDegs         minAngleDegs)
{
  // -------------------------------------------------------------
  // Terrain blockage creation
  if (myTerrainAlg.empty()) {
    LogInfo("No terrain blockage algorithm requested.\n");
    return nullptr;
  } else {
    std::shared_ptr<TerrainBlockage>
    myTerrainBlockage = TerrainBlockage::createFromCommandLineOption(myTerrainAlg,
        radarLocation, radarRangeKMs, radarName, minTerrainKMs, minAngleDegs);
    // Stubbornly refuse to run if terrain requested by name and not found or failed
    if (myTerrainBlockage == nullptr) {
      LogSevere("Terrain blockage '" << myTerrainAlg << "' requested, but failed to find and/or initialize.\n");
      exit(1);
    } else {
      LogInfo("Using Terrain Blockage: '" << myTerrainBlockage << "'\n");
      return myTerrainBlockage;
    }
    return nullptr;
  }
}

bool
PluginVolume::declare(RAPIOProgram * owner, const std::string& name)
{
  Volume::introduceSelf();
  owner->addPlugin(new PluginVolume(name));
  return true;
}

void
PluginVolume::declareOptions(RAPIOOptions& o)
{
  // Elevation volume plugin
  o.optional(myName, "simple",
    "Volume algorithm, such as 'simple', or your own. Params follow: simple,params.");
  Volume::introduceSuboptions(myName, o);
}

void
PluginVolume::addPostLoadedHelp(RAPIOOptions& o)
{
  o.addAdvancedHelp(myName, Volume::introduceHelp());
}

void
PluginVolume::processOptions(RAPIOOptions& o)
{
  myVolumeAlg = o.getString(myName);
}

void
PluginVolume::execute(RAPIOProgram * caller)
{
  // Nothing, a unique terrain is lazy requested with extra params
}

std::shared_ptr<Volume>
PluginVolume::getNewVolume(
  const std::string& historyKey)
{
  // Elevation volume creation
  LogInfo(
    "Creating virtual volume for '" << historyKey << "\n");
  std::shared_ptr<Volume> myVolume =
    Volume::createFromCommandLineOption(myVolumeAlg, historyKey);

  // Stubbornly refuse to run if requested by name and not found or failed
  if (myVolume == nullptr) {
    LogSevere("Volume '" << myVolumeAlg << "' requested, but failed to find and/or initialize.\n");
    exit(1);
  } else {
    LogInfo("Using Volume algorithm: '" << myVolumeAlg << "'\n");
  }
  return myVolume;
}
