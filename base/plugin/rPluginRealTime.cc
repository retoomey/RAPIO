#include "rPluginRealTime.h"

#include <rRAPIOAlgorithm.h>

using namespace rapio;
using namespace std;

bool
PluginRealTime::declare(RAPIOProgram * owner, const std::string& name)
{
  owner->addPlugin(new PluginRealTime(name));
  return true;
}

void
PluginRealTime::declareOptions(RAPIOOptions& o)
{
  const std::string r = "r";

  o.optional(myName, "old", "Read mode for algorithm.");
  o.addGroup(myName, "TIME");
  o.addSuboption(myName, "old", "Only process existing old records and stop.");
  o.addSuboption(myName, "new", "Only process new incoming records. Same as -r");
  o.addSuboption(myName, "", "Same as new, original realtime flag.");
  o.addSuboption(myName, "all", "All old records, then wait for new.");
}

void
PluginRealTime::addPostLoadedHelp(RAPIOOptions& o)
{
  o.addAdvancedHelp(myName,
    "Controls how records from inputs are handled. For example, reading a directory might be an archive process where you process all records and then stop.  Or you only process new incoming records.  Or some combination of both.");
}

void
PluginRealTime::processOptions(RAPIOOptions& o)
{
  myReadMode = o.getString(myName);
}

void
PluginRealTime::execute(RAPIOProgram * caller)
{
  myActive = true;
}

bool
PluginRealTime::isDaemon()
{
  return ((myReadMode == "") || (myReadMode == "new") || (myReadMode == "all"));
}

bool
PluginRealTime::isArchive()
{
  return (myReadMode == "old") || (myReadMode == "all");
}
