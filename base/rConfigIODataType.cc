#include "rConfigIODataType.h"

#include "rStrings.h"
#include "rError.h"
#include "rFactory.h"
#include "rIODataType.h"

#include <memory>

using namespace rapio;

bool ConfigIODataType::myUseSubDirs = true;
std::map<std::string, std::shared_ptr<PTreeNode> > ConfigIODataType::myDatabase;
std::map<std::string, std::string> ConfigIODataType::mySuffixes;

void
ConfigIODataType::introduceSelf()
{
  std::shared_ptr<ConfigIODataType> t = std::make_shared<ConfigIODataType>();
  Config::introduce("datatype", t);
}

bool
ConfigIODataType::readSettings(std::shared_ptr<PTreeData> d)
{
  try{
    // This should be done by config higher 'up' I think...
    // as we add sections to global configuration
    // This should be first thing passed to us I think....
    auto topTree  = d->getTree()->getChild("settings");
    auto datatype = topTree.getChildOptional("datatype");
    if (datatype != nullptr) {
      // ----------------------------------------------------------
      // <suffixes> database
      auto suffixes = datatype->getChildOptional("suffixes");
      if (suffixes != nullptr) {
        auto files = suffixes->getChildrenPtr("file"); // Actual nodes...
        for (auto f: files) {
          const auto suffix = f->getAttr("suffix", std::string(""));
          const auto io     = f->getAttr("io", std::string(""));
          mySuffixes[suffix] = io;
        }
      }

      // ----------------------------------------------------------
      // <io> database
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
              // Lazy load assign the module, it will be loaded on demand
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

std::shared_ptr<PTreeNode>
ConfigIODataType::getSettings(const std::string& key)
{
  return myDatabase[key];
}

std::string
ConfigIODataType::getIODataTypeFromSuffix(const std::string& suffix)
{
  return mySuffixes[suffix];
}
