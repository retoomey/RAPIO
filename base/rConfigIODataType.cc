#include "rConfigIODataType.h"

#include "rStrings.h"
#include "rError.h"
#include "rFactory.h"
#include "rIODataType.h"

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
      const std::string create = "createRAPIOIO";
      // Children know their own settings, we just keep the node for them.
      auto ioset = datatype->getChildrenPtr("io"); // Actual nodes...
      for (auto io: ioset) {
        // names = "netcdf netcdf3" for example can share settings
        auto names = io->getAttr("names", std::string(""));
        // module = "libsomething.so" for dynamic modules
        auto modulename = io->getAttr("module", std::string(""));
        if (!names.empty()) {
          std::vector<std::string> pieces;
          Strings::splitWithoutEnds(names, ' ', &pieces);
          for (auto& n:pieces) {
            myDatabase[n] = io;
            if (!modulename.empty()) {
              // Since it's lazy loaded we'll add it directly here
              // FIXME: Maybe cleaner to pull/load in algorithm?
              Factory<IODataType>::introduceLazy(n, modulename, "createRAPIOIO");
            }
          }
        }
      }
    } else {
      LogSevere("No datatype settings provided, will be using defaults for read/write\n");
    }
  }catch (const std::exception& e) {
    LogSevere("Error parsing datatype settings\n");
  }
  return true;
} // ConfigIODataType::readSettings

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
