#include <rPluginHeartbeat.h>

#include <rHeartbeat.h>
#include <rRAPIOAlgorithm.h>
#include <rEventLoop.h>

using namespace rapio;
using namespace std;

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
    "Sends heartbeat to program.  Note if your program lags due to calculation/write times, you may miss a heartbeat. The format is a 6 star second supported cronlist.\nCommon use cases:\n    '*/10 * * * * *' -- Run every 10 seconds.  Note max seconds is 59 here.\n    '0  */2 * * * *' -- Run every 2 minutes at the 0th second.");
}

void
PluginHeartbeat::processOptions(RAPIOOptions& o)
{
  myCronList = o.getString(myName);
}

void
PluginHeartbeat::postRecordEvent(RAPIOProgram * caller, const Record& rec)
{
  // Call a heartbeat check after every record, this
  // will make archive work (no timer)
  if (myHeartBeat && (caller->isArchive())) {
    myHeartBeat->checkForPulse();
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
      LogSevere("Bad format for -" << myName << " string, aborting\n");
      exit(1);
    }

    // Note: Need -r to be daemon, even if using ifam in daemon mode
    // FIXME: -r needs some more work.
    // We're gonna do a !archive here instead of daemon
    bool daemon = !(caller->isArchive());
    if (daemon) {
      // Wrap with a timer, it will call the heartbeat for us
      std::shared_ptr<HeartbeatTimer> timer = nullptr;
      timer = std::make_shared<HeartbeatTimer>(myHeartBeat, 1000);
      EventLoop::addEventHandler(timer);
    }
    myActive = true;
  }
}
