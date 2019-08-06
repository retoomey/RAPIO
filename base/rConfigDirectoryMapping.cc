#include "rConfigDirectoryMapping.h"

#include "rStrings.h"
#include "rIOXML.h"

#include <memory>

using namespace rapio;

std::map<std::string, std::string> ConfigDirectoryMapping::myMappings;

bool
ConfigDirectoryMapping::readInSettings()
{
  std::shared_ptr<XMLDocument> doc = Config::huntXMLDocument("misc/directoryMapping.xml");

  if (doc != nullptr) {
    const XMLElementList& mappings = doc->getRootElement().getChildren();

    for (auto& i:mappings) {
      const std::string& from(i->getAttribute("from"));
      const std::string& to(i->getAttribute("to"));
      myMappings[from] = to;
    }
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
