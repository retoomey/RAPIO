#include "rPluginVolumeValueResolver.h"

#include <rRAPIOAlgorithm.h>

using namespace rapio;
using namespace std;

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
