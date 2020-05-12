#include "rConfigDirectoryMapping.h"

#include "rStrings.h"
#include "rError.h"

#include <memory>

using namespace rapio;

std::map<std::string, std::string> ConfigDirectoryMapping::myMappings;

bool
ConfigDirectoryMapping::readInSettings()
{
  auto doc = Config::huntXML("misc/directoryMapping.xml");

  try{
    if (doc != nullptr) {
      auto tree = doc->getTree();
      auto maps = tree->getChildren("directoryMapping");
      for (const auto r: maps) {
        // Snag attributes
        const auto from = r.getAttr("from", std::string(""));
        const auto to   = r.getAttr("to", std::string(""));
      }
    }
  }catch (std::exception& e) {
    LogSevere("Error parsing XML from misc/directoryMapping.xml\n");
  }
  // We can work without any mappings at all
  return true;
}

void
ConfigDirectoryMapping::doDirectoryMapping(std::string& sub_me)
{
  for (auto& it:myMappings) {
    Strings::replace(sub_me, it.first, it.second);
  }
}
