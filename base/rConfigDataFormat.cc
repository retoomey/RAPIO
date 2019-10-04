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
  // Also need a constructor
  s.compress = true;
  s.subdirs  = true;
  s.datatype = "default";
  s.attributes["datatype"]         = "default";
  s.attributes["compression"]      = "true";
  s.attributes["compressionlevel"] = "6";
  s.attributes["subdirs"]    = "true";
  s.attributes["sparsegrid"] = "never";
  s.cdmcompliance = false;
  s.attributes["cdmcompliance"] = "false";
  s.faacompliance = false;
  s.attributes["faacompliance"] = "false";

  mySettings.push_back(std::make_shared<DataFormatSetting>(s));
}

// Every config class will have to read in its settings, but we should be
// able to do it ONCE at startup
bool
ConfigDataFormat::readInSettings()
{
  // go ahead and read the XML file
  auto doc = Config::huntXML("misc/dataformat");

  if (doc == nullptr) {
    LogDebug("No dataformat file found or readable, using built in defaults.\n");
    addFailsafe();
    return true;
  }
  bool haveDefault = false;

  try{
    if (doc != nullptr) {
      for (const auto r: doc->get_child("dataformat")) {
        if (r.first == "setting") {
          DataFormatSetting setting;

          // Snag attributes
          const auto l = r.second.get_child("<xmlattr>");
          setting.datatype = l.get("datatype", "");
          if (setting.datatype == "default") {
            haveDefault = true;
          }
          setting.format        = l.get("format", "");
          setting.compress      = l.get("compression", true);
          setting.subdirs       = l.get("subdirs", true);
          setting.cdmcompliance = l.get("cdmcompliance", false);
          setting.faacompliance = l.get("faacompliance", false);

          // Store all attributes generally.  This includes the specials above
          for (auto att: l) {
            setting.attributes[att.first.data()] = att.second.data();
          }
          mySettings.push_back(std::make_shared<DataFormatSetting>(setting));
        }
      }
    }
  }catch (std::exception& e) {
    LogSevere("Error parsing XML from misc/dataformat\n");
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
