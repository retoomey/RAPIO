#include "rPluginProductOutputFilter.h"

#include <rRAPIOAlgorithm.h>
#include <rColorTerm.h>
#include <rConfigParamGroup.h>
#include <rStrings.h>

using namespace rapio;
using namespace std;

bool
PluginProductOutputFilter::declare(RAPIOProgram * owner, const std::string& name)
{
  owner->addPlugin(new PluginProductOutputFilter(name));
  return true;
}

void
PluginProductOutputFilter::declareOptions(RAPIOOptions& o)
{
  o.optional(myName, "*", "The output products, controlling on/off and name remapping");
  o.addGroup(myName, "I/O");
}

void
PluginProductOutputFilter::addPostLoadedHelp(RAPIOOptions& o)
{
  std::string help =
    "Allows on/off and name change of output datatypes. For example, \"*\" means all products. Translating names is done by Key=Value. Keys are used to reference a particular product being written, while values can be name translations for multiple instances of an algorithm to avoid product write clashing.  For example, \"MyKey1=Ref2QC MyKey2=Vel1QC.  An algorithm can declare a key of any name, and then generate a default product name for that unless you use the = and override it.\"\n";

  // Add list of registered static products
  if (myKeys.size() > 0) {
    help += ColorTerm::blue() + "Registered product output keys:" + ColorTerm::reset() + "\n";
    for (size_t i = 0; i < myKeys.size(); ++i) {
      help += " " + ColorTerm::red() + myKeys[i] + ColorTerm::reset() + " : " + myKeyHelp[i] + "\n";
    }
  }
  o.addAdvancedHelp(myName, help);
}

void
PluginProductOutputFilter::processOptions(RAPIOOptions& o)
{
  ConfigParamGroupO paramO;

  paramO.readString(o.getString(myName));
}

bool
PluginProductOutputFilter::isProductWanted(const std::string& key)
{
  const auto& outputs = ConfigParamGroupO::getProductOutputInfo();

  for (auto& I:outputs) {
    std::string p = I.product;
    std::string star;

    if (Strings::matchPattern(p, key, star)) {
      return true;
    }
  }

  return false;
}

std::string
PluginProductOutputFilter::resolveProductName(const std::string& key,
  const std::string                                            & productName)
{
  std::string newProductName = productName;

  const auto& outputs = ConfigParamGroupO::getProductOutputInfo();

  for (auto& I:outputs) {
    // If key is found...
    std::string star;
    if (Strings::matchPattern(I.product, key, star)) {
      // ..translate the productName if there's a "Key=translation"
      std::string p2 = I.toProduct;
      if (p2 != "") {
        Strings::replace(p2, "*", star);
        newProductName = p2;
      }
      break;
    }
  }

  return newProductName;
}
