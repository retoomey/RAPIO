#include "rPluginNotifier.h"

#include <rRAPIOAlgorithm.h>
#include <rConfigParamGroup.h>

using namespace rapio;
using namespace std;

std::vector<std::shared_ptr<RecordNotifierType> > PluginNotifier::theNotifiers;

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
