#include "rRAPIOAlgorithm.h"

#include "rRAPIOOptions.h"
#include "rError.h"
#include "rEventLoop.h"
#include "rTime.h"
#include "rStrings.h"
#include "rDataType.h"
#include "rIODataType.h"
#include "rProcessTimer.h"
#include "rRecordQueue.h"
#include "rFactory.h"
#include "rDataTypeHistory.h"
#include "rConfigParamGroup.h"

// Plugins algorithms use by default
#include "rRAPIOPlugin.h"
#include "rPluginHeartbeat.h"
#include "rPluginWebserver.h"
#include "rPluginNotifier.h"
#include "rPluginIngestor.h"
#include "rPluginRecordFilter.h"
#include "rPluginProductOutput.h"
#include "rPluginProductOutputFilter.h"
#include "rPluginRealTime.h"

#include <iostream>
#include <string>

using namespace rapio;
using namespace std;

TimeDuration RAPIOAlgorithm::myMaximumHistory;

void
RAPIOAlgorithm::initializeOptions(RAPIOOptions& o)
{
  // FIXME: These will become plugins too I think.  Eventually all algorithms/programs
  // will just declare plugin abilities.  Little by little

  // All stock algorithms have a optional history setting.  We can overload this
  // in time
  // if more precision is needed.
  o.optional("h", "900", "History in seconds kept for inputs source(s).");
  o.addGroup("h", "TIME");

  o.optional("postwrite",
    "",
    "Simple executable to call post file writing using %filename%.");
  o.addGroup("postwrite", "I/O");
  o.setHidden("postwrite");
  o.optional("postfml",
    "",
    "Simple executable to call post FML file writing using %filename%.");
  o.addGroup("postfml", "I/O");
  o.setHidden("postfml");

  return RAPIOProgram::initializeOptions(o);
}

void
RAPIOAlgorithm::addPostLoadedHelp(RAPIOOptions& o)
{
  // Static advanced on demand (lazy add)
  o.addAdvancedHelp("h",
    "For indexes, this will be the global time in minutes for all indexes provided.");
  o.addAdvancedHelp("postwrite",
    "Allows you to run a command on a file output file. The 'ldm' command maps to 'pqinsert -v -f EXP %filename%', but any command in path can be ran using available macros.  Example: 'file %filename%' or 'ldm' or 'aws cp %filename'.");

  o.addAdvancedHelp("postfml",
    "Allows you to run a command on a FML output file. The 'ldm' command maps to 'pqinsert -v -f EXP %filename%', but any command in path can be ran using available macros.  Example: 'file %filename%' or 'ldm' or 'aws cp %filename'.");
  // Now let subclasses declare more things.
  // We do it this way to keep the algorithms from having to call superclass first
  declareAdvancedHelp(o);
}

TimeDuration
RAPIOAlgorithm::getMaximumHistory()
{
  return (myMaximumHistory);
}

void
RAPIOAlgorithm::purgeTimeWindow()
{
  DataTypeHistory::purgeTimeWindow(Time::CurrentTime());
}

bool
RAPIOAlgorithm::inTimeWindow(const Time& aTime)
{
  const TimeDuration window = Time::CurrentTime() - aTime;

  // LogSevere("WINDOW FOR TIME: " << aTime << ", " << Time::CurrentTime() << " ---> " << window << " MAX? " << myMaximumHistory << "\n");
  return (window <= myMaximumHistory);
}

bool
RAPIOAlgorithm::isDaemon()
{
  auto r = getPlugin<PluginRealTime>("r");

  if (r) {
    return r->isDaemon();
  }
  return false;
}

bool
RAPIOAlgorithm::isArchive()
{
  auto r = getPlugin<PluginRealTime>("r");

  if (r) {
    return (r->isArchive());
  }
  return false;
}

void
RAPIOAlgorithm::initializePlugins()
{
  // Stock plugins for Algorithm we setup.  Most user algorithms
  // will want these things, so we default to having them rather
  // than forcing subclass to call us to setup.
  // FIXME: We could not add them unless wanted in the particular
  // algorithm, might be better to be consistent though.
  PluginRealTime::declare(this, "r");     // Record mode ability
  PluginHeartbeat::declare(this, "sync"); // Heartbeat ability/options
  PluginWebserver::declare(this, "web");  // Webserver ability/options
  // I/O plugins
  PluginNotifier::declare(this, "n");            // Notification ability (fam, etc.)
  PluginRecordFilter::declare(this, "I");        // Record filter ability (ingest)
  PluginIngestor::declare(this, "i");            // Ingestor index ability and record queue
  PluginProductOutputFilter::declare(this, "O"); // Product filter ability (output)
  PluginProductOutput::declare(this, "o");       // Output plugin ability

  return RAPIOProgram::initializePlugins();
}

void
RAPIOAlgorithm::finalizeOptions(RAPIOOptions& o)
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

  // Gather postwrite option
  myPostWrite = o.getString("postwrite");
  myPostFML   = o.getString("postfml");

  return RAPIOProgram::finalizeOptions(o);
}

void
RAPIOAlgorithm::execute()
{
  // Plugins
  for (auto p: myPlugins) {
    p->execute(this);
  }

  // Set the time mode for system.(post plugin setup)
  Time::setArchiveMode(!isDaemon());

  // Time until end of program.  Make it dynamic memory instead of heap or
  // the compiler will kill it too early (too smartz)
  static std::shared_ptr<ProcessTimer> fulltime(
    new ProcessTimer("Algorithm total runtime"));

  EventLoop::doEventLoop();
} // RAPIOAlgorithm::execute

void
RAPIOAlgorithm::handleRecordEvent(const Record& rec)
{
  // Always just keep the latest data time as 'current time'
  // for archive.  Or real clock in real time.
  Time::setLatestDataTime(rec.getTime());

  // Handle 'data' messages/records with filenames
  if (rec.isData()) {
    purgeTimeWindow();

    RAPIOData d(rec);

    // DataTypeHistory might store the created DataType, we
    // don't want to double create if the alg uses it also.
    DataTypeHistory::processNewData(d);

    // Now the algorithm can also process if wanted
    processNewData(d);
  }

  // Handle messages.  Note: It is intended that a data
  // record with added messages get sent here.
  if (rec.isMessage()) {
    processNewMessage(rec);
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
    EventLoop::exit(0);
  } else {
    // Realtime queue is empty, hey we're caught up...
  }
}

void
RAPIOAlgorithm::declareProduct(const std::string& key, const std::string& help)
{
  // Use the plugin if we have it
  auto p = getPlugin<PluginProductOutputFilter>("O");

  if (p) {
    return p->declareProduct(key, help);
  }
}

bool
RAPIOAlgorithm::isProductWanted(const std::string& key)
{
  // Use the plugin if we have it
  auto p = getPlugin<PluginProductOutputFilter>("O");

  if (p) {
    return p->isProductWanted(key);
  }
  return true; // allow if no plugin support
}

std::string
RAPIOAlgorithm::resolveProductName(const std::string& key, const std::string& defaultName)
{
  // Use the plugin if we have it
  auto p = getPlugin<PluginProductOutputFilter>("O");

  if (p) {
    return p->resolveProductName(key, defaultName);
  }
  return defaultName;
}

void
RAPIOAlgorithm::writeOutputMessage(const Message& m_in,
  std::map<std::string, std::string>            & outputParams)
{
  // Try to make this a bit unique to avoid any stepping over issues from
  // other algorithms.
  auto m = m_in; // FIXME: possibly wasteful.  Not wanting to set the
  // source by default though.
  std::stringstream n;

  n << "M_" << OS::getHostName() << "_" << OS::getProcessName(true) << "_" << OS::getProcessID();
  m.setSourceName(n.str());

  // We need output folders to send the message to.  For now, assuming
  // each product output location.  However each IODataType can generate this
  // with handleCommandParam.  Since we're not writing a DataType we don't
  // have the subclass.  The only one currently different is the IOPython, which
  // splits the field up.
  // FIXME: I think when we 'eventually' refactor the -i -O we will have
  // to revisit this. Or we do a clean up pass/api improvement here.
  //
  const auto& writers = ConfigParamGroupo::getWriteOutputInfo();

  for (auto& w:writers) {
    // Hardset writer to one only...this requires a writer=/path in -o to work
    // For example 2D fusion forces hmrg binary by -o hmrg=/path and setting onewriter to hmrg
    if (!outputParams["onewriter"].empty()) {
      if (w.factory != outputParams["onewriter"]) {
        continue;
      }
    }
    outputParams["outputfolder"] = w.outputinfo; // Hack: w.outputinfo is processed by handleCommandParam

    // Send to each notifier.  Let the notifier promote to record if wanted
    for (auto& n:PluginNotifier::theNotifiers) {
      n->writeMessage(outputParams, m);
    }

    // Path with datatype, setting outputfolder per output info:
    // IODataType::write(outputData, w.outputinfo, records, w.factory, outputParams);
    //    --> write1(dt, outputinfo, records, f, outputParams);
    //    auto encoder  = getFactory(f, outputinfo, dt);
    //    encoder->writeout(dt, outputinfo, records, f, outputParams);
    //    encoder->handleCommandParam(outputinfo);
  }
} // RAPIOAlgorithm::writeOutputMessage

void
RAPIOAlgorithm::writeOutputProduct(const std::string& key,
  std::shared_ptr<DataType>                         outputData,
  std::map<std::string, std::string>                & outputParams)
{
  outputParams["filepathmode"] = "datatype";
  outputParams["postwrite"]    = myPostWrite;
  outputParams["postfml"]      = myPostFML;

  if (isProductWanted(key)) {
    // Original typeName, which may match key or not
    const std::string typeName = outputData->getTypeName();

    // Resolve using the "-O key=resolved, if exists"
    const std::string newProductName = resolveProductName(key, typeName);

    // Write DataType with given typename, or optionally filtered to new typename by -O
    const bool changeProductName = (typeName != newProductName);
    if (changeProductName) {
      LogInfo("Writing '" << typeName << "' as product name '" << newProductName
                          << "'\n");
      outputData->setTypeName(newProductName);
    }

    const auto& writers = ConfigParamGroupo::getWriteOutputInfo();
    for (auto& w:writers) {
      // Hardset writer to one only...this requires a writer=/path in -o to work
      // For example 2D fusion forces hmrg binary by -o hmrg=/path and setting onewriter to hmrg
      if (!outputParams["onewriter"].empty()) {
        if (w.factory != outputParams["onewriter"]) {
          continue;
        }
      }

      // Can call write multiple times for each output wanted.
      std::vector<Record> records;
      const bool success = IODataType::write(outputData, w.outputinfo, records, w.factory, outputParams);

      if (success) { // Only notify iff the file writes successfully
        // Get back the output folder for notifications
        // and notify each notifier for this writer.
        for (auto& n:PluginNotifier::theNotifiers) { // if any, use
          n->writeRecords(outputParams, records);
        }
      } // Not gonna error..writers should be complaining
    }

    // Restore original typename, does matter since DataType might be reused.
    outputData->setTypeName(typeName);
  } else {
    LogInfo("Skipping write for -O unwanted product '" << key << "'\n");
  }
} // RAPIOAlgorithm::writeOutputProduct
