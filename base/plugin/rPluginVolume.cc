#include "rPluginVolume.h"

#include <rRAPIOAlgorithm.h>
#include <rStrings.h>

using namespace rapio;
using namespace std;

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
  fLogInfo("Creating virtual volume for '{}'", historyKey);

  // We break it up into key and the params
  std::string key, params;

  Strings::splitKeyParam(myVolumeAlg, key, params);

  std::shared_ptr<Volume> myVolume =
    Volume::createVolume(key, params, historyKey);

  // Stubbornly refuse to run if requested by name and not found or failed
  if (myVolume == nullptr) {
    fLogSevere("Volume '{}' requested, but failed to find and/or initialize.", myVolumeAlg);
    exit(1);
  } else {
    fLogInfo("Using Volume algorithm: '{}'", myVolumeAlg);
  }
  return myVolume;
}
