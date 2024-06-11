#include "rPluginProductOutput.h"

#include <rRAPIOAlgorithm.h>
#include <rIODataType.h>
#include <rConfigParamGroup.h>

using namespace rapio;
using namespace std;

bool
PluginProductOutput::declare(RAPIOProgram * owner, const std::string& name)
{
  owner->addPlugin(new PluginProductOutput(name));
  return true;
}

void
PluginProductOutput::declareOptions(RAPIOOptions& o)
{
  // These are the standard generic 'output' parameters we support
  o.require("o", "/data", "The output writers/directory for generated products or data");
  o.addGroup("o", "I/O");
}

void
PluginProductOutput::addPostLoadedHelp(RAPIOOptions& o)
{
  // Datatype will try to load all dynamic modules, so we want to avoid that
  // unless help is requested
  if (o.wantAdvancedHelp("o")) {
    o.addAdvancedHelp("o", IODataType::introduceHelp());
  }
}

void
PluginProductOutput::processOptions(RAPIOOptions& o)
{
  ConfigParamGroupo paramO;

  paramO.readString(o.getString(myName));
}
