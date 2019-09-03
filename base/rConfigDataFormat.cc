#include "rConfigDataFormat.h"
#include "rError.h"
#include "rIOXML.h"
#include "rFactory.h"
#include "rDataType.h"

#include <memory>

using namespace rapio;

std::vector<std::shared_ptr<DataFormatSetting> > ConfigDataFormat::mySettings;

ConfigDataFormat::ConfigDataFormat()
{ }

void
ConfigDataFormat::addFailsafe()
{
  DataFormatSetting s;

  // This is what it belongs to.
  s.attributes["format"] = "netcdf";
  s.format = "netcdf";

  // Defaults for a netcdf output writer.
  // Note, each of the writers can have whatever they want
  // FIXME: probably don't need special flag fields
  s.compress = true;
  s.subdirs  = true;
  s.datatype = "default";
  s.attributes["datatype"]         = "default";
  s.attributes["compression"]      = "true";
  s.attributes["compressionlevel"] = "6";
  s.attributes["subdirs"]       = "true";
  s.attributes["sparsegrid"]    = "never";
  s.attributes["cdmcompliance"] = "false";
  s.attributes["faacompliance"] = "false";

  mySettings.push_back(std::make_shared<DataFormatSetting>(s));
}

// Every config class will have to read in its settings, but we should be
// able to do it ONCE at startup
bool
ConfigDataFormat::readInSettings()
{
  // go ahead and read the XML file
  std::shared_ptr<XMLDocument> doc =
    Config::huntXMLDocument("misc/dataformat");

  if (doc == nullptr) {
    LogDebug("No dataformat file found or readable, using built in defaults.\n");
    addFailsafe();
    return true;
  }

  std::vector<std::shared_ptr<XMLElement> > setting_elements;
  doc->getRootElement().getChildren("setting", &setting_elements);

  bool haveDefault = false;
  for (auto& i: setting_elements) {
    DataFormatSetting setting;
    const std::string& datatype = i->getAttribute("datatype");
    setting.datatype = datatype;

    LogDebug("Reading dataformat settings..." << datatype << "\n");
    setting.compress = true;
    i->readValidAttribute("compression", setting.compress);

    setting.subdirs = true;
    i->readValidAttribute("subdirs", setting.subdirs);

    setting.cdmcompliance = false;
    i->readValidAttribute("cdmcompliance", setting.cdmcompliance);

    setting.faacompliance = false;
    i->readValidAttribute("faacompliance", setting.faacompliance);

    std::string format = i->getAttributeValue("format",
        "netcdf");
    setting.format = format;

    setting.attributes = i->getAttributes();

    mySettings.push_back(std::make_shared<DataFormatSetting>(setting));
    if (datatype == "default") { haveDefault = true; }
  }

  if (!haveDefault) {
    LogSevere("ERROR: misc/dataformat has no setting for datatype=default. Creating default.\n");
    addFailsafe();
  }
  return true;
} // ConfigDataFormat::readInSettings

std::shared_ptr<DataFormatSetting>
ConfigDataFormat::getSetting(
  const std::string& datatype)
{
  std::shared_ptr<DataFormatSetting> d;
  for (auto& i: mySettings) {
    if (i->datatype == datatype) {
      return i;
    }
    if (i->datatype == "default") {
      d = i;
    }
  }
  return d;
}
