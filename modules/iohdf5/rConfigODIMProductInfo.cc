#include "rConfigODIMProductInfo.h"

using namespace rapio;

const std::string ConfigODIMProductInfo::ConfigODIMProductInfoXML = "misc/odimProdInfo.xml";
bool ConfigODIMProductInfo::myReadSettings = false;
std::map<std::string, ODIMProductInfo> ConfigODIMProductInfo::myProductInfos;

void
ConfigODIMProductInfo::loadSettings()
{
  // Already tried to read
  if (myReadSettings) { return; }

  auto doc = Config::huntXML(ConfigODIMProductInfoXML);

  try{
    myReadSettings = true;
    size_t count = 0;
    if (doc != nullptr) {
      auto tree = doc->getTree();
      auto ODIM = tree->getChildOptional("odimProdInfo");
      if (ODIM != nullptr) {
        auto infos = ODIM->getChildren("prodInfo");
        for (const auto& r: infos) {
          // Snag attributes
          const auto prodName = r.getAttr("prodName", std::string(""));
          const auto type     = r.getAttr("type", std::string(""));
          const auto unit     = r.getAttr("unit", std::string(""));
          const auto colorMap = r.getAttr("colorMap", std::string(""));
          // LogDebug("Read " << prodName << " " << type << " " << unit << " " << colorMap << "\n");
          myProductInfos[prodName] = ODIMProductInfo(prodName, type, unit, colorMap);
          count++;
        }
      }
    } else {
      LogInfo("No " << ConfigODIMProductInfoXML << " found, ODIM products will use default settings\n");
    }
    LogInfo("Read " << count << " product infos from " << ConfigODIMProductInfoXML << "\n");
  }catch (const std::exception& e) {
    LogSevere("Error parsing XML from " << ConfigODIMProductInfoXML << "\n");
  }
} // ConfigODIMProductInfo::loadSettings

ODIMProductInfo
ConfigODIMProductInfo::getProductInfo(const std::string& name)
{
  loadSettings();
  if (myProductInfos.count(name) > 0) {
    return myProductInfos[name];
  }
  return ODIMProductInfo(name, name, "dBZ", "dBZ");
}
