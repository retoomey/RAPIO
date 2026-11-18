#include <rPluginHeartbeat.h>

#include <rHeartbeat.h>
#include <rRAPIOAlgorithm.h>
#include <rEventLoop.h>

using namespace rapio;
using namespace std;

bool
PluginHeartbeat::declare(RAPIOProgram * owner, std::string_view name)
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
    "Sends heartbeat to program.  Note if your program lags due to calculation/write times, you may miss a heartbeat. The format is a 6 star second supported cronlist.\nRealtime mode uses system clock to trigger a heartbeat.  Archive mode triggers when the latest record is 'sync' time newer than the last, and once at the end of the archive to finalize.\nCommon use cases:\n    '*/10 * * * * *' -- Run every 10 seconds.  Note max seconds is 59 here.\n    '0  */2 * * * *' -- Run every 2 minutes at the 0th second.");
}

void
PluginHeartbeat::processOptions(RAPIOOptions& o)
{
  myCronList = o.getString(myName);
}

void
PluginHeartbeat::preRecordEvent(RAPIOProgram * caller, const Record& rec)
{
  if (myHeartBeat && caller->isArchive() && rec.isData()) {
    myHeartBeat->advanceToTime(rec.getTime());
  }
}

void
PluginHeartbeat::postRecordEvent(RAPIOProgram * /*caller*/, const Record&)
{ }

void
PluginHeartbeat::endDatasetEvent(RAPIOProgram * caller)
{
  if (myHeartBeat && caller->isArchive()) {
    myHeartBeat->finalizeArchive();
  }
}

void
PluginHeartbeat::execute(RAPIOProgram * caller)
{
  myHeartBeat = nullptr;     // fixme; constructor
  if (!myCronList.empty()) { // None required, don't make it
    // Create heartbeat
    myHeartBeat = std::make_shared<Heartbeat>(caller);

    if (!myHeartBeat->setCronList(myCronList)) {
      throw StartupException(fmt::format("Bad format for -{} string.", myName));
    }

    if (caller->isDaemon()) {
      // Wrap with a timer, it will call the heartbeat for us
      auto timer = std::make_shared<HeartbeatTimer>(myHeartBeat, 1000);
      EventLoop::addEventHandler(timer);
    }
    myActive = true;
  }
}
