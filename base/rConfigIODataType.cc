#include "rConfigIODataType.h"

#include "rStrings.h"
#include "rError.h"

#include <memory>

using namespace rapio;

bool ConfigIODataType::myUseSubDirs = true;
std::map<std::string, std::shared_ptr<XMLNode> > ConfigIODataType::myDatabase;

void
ConfigIODataType::introduceSelf()
{
  std::shared_ptr<ConfigIODataType> t = std::make_shared<ConfigIODataType>();
  Config::introduce("datatype", t);
}

bool
ConfigIODataType::readSettings(std::shared_ptr<XMLData> d)
{
  try{
    // This should be done by config higher 'up' I think...
    // as we add sections to global configuration
    // This should be first thing passed to us I think....
    auto topTree  = d->getTree()->getChild("settings");
    auto datatype = topTree.getChildOptional("datatype");

    // Handle the XMLNode we're given...
    if (datatype != nullptr) {
      // FIXME: Parse 'general' datatype settings into us.
      // the use sub directories flag is a general one, right?
      //
      // Children know their own settings, we just keep the node for them.
      auto ioset = datatype->getChildrenPtr("io"); // Actual nodes...
      for (auto io: ioset) {
        auto names = io->getAttr("names", std::string(""));
        if (!names.empty()) {
          myDatabase[names] = io;
        }
      }
    } else {
      LogSevere("No datatype settings provided, will be using defaults for read/write\n");
    }
  }catch (const std::exception& e) {
    LogSevere("Error parsing datatype settings\n");
  }
  return true;
}

bool
ConfigIODataType::getUseSubDirs()
{
  return myUseSubDirs;
}

std::shared_ptr<XMLNode>
ConfigIODataType::getSettings(const std::string& key)
{
  return myDatabase[key];
}
