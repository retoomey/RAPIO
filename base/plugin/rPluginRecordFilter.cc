#include "rPluginRecordFilter.h"

#include <rRAPIOAlgorithm.h>
#include <rRecordFilter.h>
#include <rConfigParamGroup.h>

using namespace rapio;
using namespace std;

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
