#include "rConfigNIDSInfo.h"

#include "rStrings.h"
#include "rError.h"

#include <memory>

using namespace rapio;

const std::string ConfigNIDSInfo::ConfigNIDSInfoXML = "misc/NIDSProdInfo.xml";
bool ConfigNIDSInfo::myReadSettings = false;
std::unordered_map<int, NIDSInfo> ConfigNIDSInfo::myNIDSInfos;

void
ConfigNIDSInfo::introduceSelf()
{
  std::shared_ptr<ConfigNIDSInfo> d = std::make_shared<ConfigNIDSInfo>();

  Config::introduce("nidsinfo", d);
}

bool
ConfigNIDSInfo::readSettings(std::shared_ptr<PTreeData> d)
{
  // We don't use d (tags from global rapioconfig.xml), but
  // we always have that for later for overrides, etc.
  // Use a separate configuration file for our stuff

  // Note: We can lazy load on demand
  // const URL loc(Config::getAbsoluteForRelative(ConfigNIDSInfoXML));
  // if (loc.empty()){
  // This is before logging initialized
  //   LogInfo("Couldn't find " << ConfigNIDSInfoXML << "\n");
  // }

  // Force loading at startup for testing. Typically don't do this
  // since this database is decent size and not used everywhere
  // loadSettings();

  return true;
}

void
ConfigNIDSInfo::loadSettings()
{
  // Already tried to read
  if (myReadSettings) { return; }

  auto doc = Config::huntXML(ConfigNIDSInfoXML);

  try{
    myReadSettings = true;
    size_t count = 0;
    if (doc != nullptr) {
      auto tree      = doc->getTree();
      auto nidsgroup = tree->getChildOptional("NIDSProdInfo");
      if (nidsgroup != nullptr) {
        auto nids = nidsgroup->getChildren("info");

        for (auto& n: nids) {
          // Snag attributes
          const auto code     = n.getAttr("code", (int) (-1.0));
          const auto site     = n.getAttr("type", std::string(""));
          const auto res      = n.getAttr("res", std::string(""));
          const auto range    = n.getAttr("range", (int) (-1.0));
          const auto level    = n.getAttr("level", (int) (-1.0));
          const auto units    = n.getAttr("units", std::string(""));
          const auto datatype = n.getAttr("datatype", std::string(""));
          const auto compress = n.getAttr("compress", std::string(""));
          bool c = ((compress != "no") && (compress != "")); // Default off?

          // scaled thresehold products
          const auto decode   = n.getAttr("decode", (int) (1));
          const auto min      = n.getAttr("min", (int) (-1.0));
          const auto increase = n.getAttr("increase", (int) (-1.0));

          if (myNIDSInfos.count(code) > 0) {
            LogSevere("Duplicate product code found, using latest. Check xml file.\n");
          }
          myNIDSInfos[code] = NIDSInfo(code, site, res, range, level, units,
              datatype, c, decode, min, increase);
          count++;
        }
      }
    } else {
      LogInfo("No " << ConfigNIDSInfoXML << " found, no NIDS info database support.\n");
    }
    LogInfo("Read " << count << " product codes into NIDS info database\n");
  }catch (const std::exception& e) {
    LogSevere("Error parsing XML from " << ConfigNIDSInfoXML << "\n");
  }
} // ConfigNIDSInfo::loadSettings

int
ConfigNIDSInfo::haveCode(int code)
{
  loadSettings();
  return (myNIDSInfos.count(code) > 0);
}

std::string
NIDSInfo::getProductName() const
{
  std::stringstream ss;

  ss << getType() << "-" <<
    getResolution() << "-" <<
    getRange() << "-" <<
    getLevel();
  return ss.str();
}

NIDSInfo
ConfigNIDSInfo::getNIDSInfo(int code)
{
  loadSettings();
  auto it = myNIDSInfos.find(code);

  if (it != myNIDSInfos.end()) {
    return it->second;
  }
  return NIDSInfo(); // Return code of -1
}

LengthMs
NIDSInfo::getGateWidthMeters() const
{
  // FIXME: We could do this on read. This
  // is currently only for RadialSets so convert here.
  // We should generalize this if resolution is
  // more general for other products as well.
  int xPos = myResolution.find('x');
  std::string strlen = myResolution.substr(0, xPos);

  float value = 250;

  try {
    value = std::stof(strlen);
  }catch (const std::exception& e) {
    LogSevere("Non number in resolution?\n");
    return 250.0;
  }

  if (myCode != 34) { // product 34 is a special case
    // Nautical Miles to Meters
    return value * 1852.0f; // magic
  } else {
    // Kilometers to Meters
    return value * 1000.0f;
  }
}

std::ostream&
rapio::operator << (std::ostream& os, const NIDSInfo& n)
{
  return (os << "<info" <<
         " code=\"" << n.myCode << "\"" <<
         " type=\"" << n.myType << "\"" <<
         " res=\"" << n.myResolution << "\"" <<
         " range=\"" << n.myRange << "\"" <<
         " level=\"" << n.myLevel << "\"" <<
         ">");
}
