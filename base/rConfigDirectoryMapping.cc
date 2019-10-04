#include "rConfigDirectoryMapping.h"

#include "rStrings.h"
#include "rIOXML.h"
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
      for (const auto r: doc->get_child("directoryMapping")) {
        // Snag attributes
        const auto l = r.second.get_child("<xmlattr>");
        // const auto from = r.second.get("<xmlattr>.from", "");
        const auto from = l.get("from", "");
        const auto to   = l.get("to", "");
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
