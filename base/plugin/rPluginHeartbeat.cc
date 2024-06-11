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
