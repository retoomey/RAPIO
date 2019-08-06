#include "rConfigDataFormat.h"
#include "rError.h"
#include "rIOXML.h"
#include "rFactory.h"
#include "rDataType.h"

#include <memory>

using namespace rapio;

std::map<std::string, std::shared_ptr<DataFormatSetting> > ConfigDataFormat::mySettings;

ConfigDataFormat::ConfigDataFormat()
{ }

// Every config class will have to read in its settings, but we should be
// able to do it ONCE at startup
bool
ConfigDataFormat::readInSettings()
{
  // go ahead and read the XML file
  std::shared_ptr<XMLDocument> doc =
    Config::huntXMLDocument("misc/dataformat");

  if (doc == 0) {
    return false; // We have to have it
  }

  std::vector<std::shared_ptr<XMLElement> > setting_elements;
  doc->getRootElement().getChildren("setting", &setting_elements);

  for (auto& i: setting_elements) {
    DataFormatSetting setting;
    const std::string& datatype = i->getAttribute("datatype");

    LogDebug("Reading stuff from dataformat..." << datatype << "\n");
    setting.compress = true;
    i->readValidAttribute("compression", setting.compress);

    setting.subdirs = true;
    i->readValidAttribute("subdirs", setting.subdirs);

    std::string format = i->getAttributeValue("format",
        "netcdf");

    // Feel like this 'shouldn't' be here but ok we'll leave for now
    setting.prototype = Factory<IODataType>::get(format,
        "IODataType settings init");
    setting.attributes = i->getAttributes();

    if (setting.prototype != nullptr) {
      mySettings[datatype] =
        std::shared_ptr<DataFormatSetting>(new DataFormatSetting(setting));
    } else {
      LogDebug("Prototype is null for " << datatype << "\n");
    }
  }

  if (mySettings.find("default") == mySettings.end()) {
    LogSevere("ERROR: misc/dataformat has no setting for datatype=default. "
      << "So basically we can't process any data.\n");
    return false;
  }
  return true;
} // ConfigDataFormat::readInSettings

std::shared_ptr<DataFormatSetting>
ConfigDataFormat::getSetting(
  const std::string& datatype)
{
  auto iter = mySettings.find(datatype);

  if (iter == mySettings.end()) {
    iter = mySettings.find("default");
  }

  if (iter != mySettings.end()) {
    return (iter->second);
  }
  return (0);
}
