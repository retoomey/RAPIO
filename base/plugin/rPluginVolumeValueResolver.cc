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
  o.optional("rparams", "",
    "Param list passed onto the choosen resolver algorithm.");
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
  std::string key, params;

  Strings::splitKeyParam(myResolverAlg, key, params);
  myResolver = VolumeValueResolver::createVolumeValueResolver(key, params);

  // Stubbornly refuse to run if Volume Value Resolver requested by name and not found or failed
  if (myResolver == nullptr) {
    fLogSevere("Volume Value Resolver '{}' requested, but failed to find and/or initialize.", myResolverAlg);
    exit(1);
  } else {
    fLogInfo("Using Volume Value Resolver: '{}'", myResolverAlg);
  }
}

std::shared_ptr<VolumeValueResolver>
PluginVolumeValueResolver::getVolumeValueResolver()
{
  return myResolver;
}
