#include "rPluginTerrainBlockage.h"

#include <rRAPIOAlgorithm.h>
#include <rStrings.h>

using namespace rapio;
using namespace std;

bool
PluginTerrainBlockage::declare(RAPIOProgram * owner, const std::string& name)
{
  TerrainBlockage::introduceSelf(); // static
  owner->addPlugin(new PluginTerrainBlockage(name));
  return true;
}

void
PluginTerrainBlockage::declareOptions(RAPIOOptions& o)
{
  // Terrain blockage plugin
  // FIXME: Lak is introduced right...so where is this declared generically?
  o.optional(myName, "", // default is none
    "Terrain blockage algorithm. Params follow: lak,/DEMS.  Most take root folder of DEMS.");
  o.addSuboption(myName, "", "Don't apply any terrain algorithm.");
  TerrainBlockage::introduceSuboptions("terrain", o);
}

void
PluginTerrainBlockage::addPostLoadedHelp(RAPIOOptions& o)
{
  o.addAdvancedHelp(myName, TerrainBlockage::introduceHelp());
}

void
PluginTerrainBlockage::processOptions(RAPIOOptions& o)
{
  myTerrainAlg = o.getString(myName);
}

void
PluginTerrainBlockage::execute(RAPIOProgram * caller)
{
  // Nothing, a unique terrain is lazy requested with extra params
}

std::shared_ptr<TerrainBlockage>
PluginTerrainBlockage::getNewTerrainBlockage(
  const LLH         & radarLocation,
  const LengthKMs   & radarRangeKMs,
  const std::string & radarName,
  LengthKMs         minTerrainKMs, // do we need these?
  AngleDegs         minAngleDegs)
{
  // -------------------------------------------------------------
  // Terrain blockage creation
  if (myTerrainAlg.empty()) {
    LogInfo("No terrain blockage algorithm requested.\n");
    return nullptr;
  } else {
    std::string key, params;
    Strings::splitKeyParam(myTerrainAlg, key, params);

    std::shared_ptr<TerrainBlockage>
    myTerrainBlockage = TerrainBlockage::createTerrainBlockage(key, params,
        radarLocation, radarRangeKMs, radarName, minTerrainKMs, minAngleDegs);

    // Stubbornly refuse to run if terrain requested by name and not found or failed
    if (myTerrainBlockage == nullptr) {
      LogSevere("Terrain blockage '" << myTerrainAlg << "' requested, but failed to find and/or initialize.\n");
      exit(1);
    } else {
      LogInfo("Using Terrain Blockage: '" << myTerrainBlockage << "'\n");
      return myTerrainBlockage;
    }
    return nullptr;
  }
}
