#include "rConfigRadarInfo.h"

#include "rStrings.h"
#include "rError.h"

#include <memory>

using namespace rapio;

const std::string ConfigRadarInfo::ConfigRadarInfoXML = "misc/radarinfo.xml";
bool ConfigRadarInfo::myReadSettings = false;
std::map<std::string, RadarInfo> ConfigRadarInfo::myRadarInfos;

void
ConfigRadarInfo::introduceSelf()
{
  std::shared_ptr<ConfigRadarInfo> d = std::make_shared<ConfigRadarInfo>();

  Config::introduce("radarinfo", d);
}

bool
ConfigRadarInfo::readSettings(std::shared_ptr<PTreeData> d)
{
  // We don't use d (tags from global rapioconfig.xml), but
  // we always have that for later for overrides, etc.
  // Use a separate configuration file for our stuff

  // Note: We can lazy load on demand
  // const URL loc(Config::getAbsoluteForRelative(ConfigRadarInfoXML));
  // if (loc.empty()){
  // This is before logging initialized
  //   LogInfo("Couldn't find " << ConfigRadarInfoXML << "\n");
  // }

  // Force loading at startup for testing. Typically don't do this
  // since this database is decent size and not used everywhere
  // loadSettings();

  return true;
}

void
ConfigRadarInfo::loadSettings()
{
  // Already tried to read
  if (myReadSettings) { return; }

  auto doc = Config::huntXML(ConfigRadarInfoXML);

  try{
    myReadSettings = true;
    size_t count = 0;
    if (doc != nullptr) {
      auto tree       = doc->getTree();
      auto radargroup = tree->getChildOptional("radars");
      if (radargroup != nullptr) {
        auto radars = radargroup->getChildren("radar");

        for (auto& r: radars) {
          // Snag attributes
          const auto name         = r.getAttr("name", std::string(""));
          const auto site         = r.getAttr("site", std::string(""));
          const auto rpgid        = r.getAttr("rpgid", (int) (-1.0));
          const auto frequency    = r.getAttr("frequency", (int) (-1.0));
          const auto polarization = r.getAttr("polarization", std::string("u"));
          const auto freqband     = r.getAttr("freqband", std::string("U"));
          const auto beamwidth    = r.getAttr("beamwidth", (float) (-1.0f));
          const auto magnetic     = r.getAttr("magnetic_offset", std::string(""));

          auto l = r.getChildOptional("location");
          if (l != nullptr) {
            const auto lat = l->getAttr("lat", 1.0f);
            const auto lon = l->getAttr("lon", 1.0f);
            const auto ht  = l->getAttr("ht", 1.0f);
            LLH aLLH(lat, lon, ht / 1000.0f);
            myRadarInfos[name] = RadarInfo(name, site, rpgid, frequency, polarization, freqband, beamwidth, aLLH,
                magnetic);
          }
          count++;
        }
      }
    } else {
      LogInfo("No " << ConfigRadarInfoXML << " found, no radar info database support.\n");
    }
    LogInfo("Read " << count << " radars into radar info database\n");
  }catch (const std::exception& e) {
    LogSevere("Error parsing XML from " << ConfigRadarInfoXML << "\n");
  }
} // ConfigRadarInfo::loadSettings

bool
ConfigRadarInfo::haveRadar(const std::string& name)
{
  loadSettings();
  return (myRadarInfos.count(name) > 0);
}

std::string
ConfigRadarInfo::getSite(const std::string& name)
{
  loadSettings();
  if (myRadarInfos.count(name) > 0) {
    return myRadarInfos[name].site;
  }
  return std::string();
}

int
ConfigRadarInfo::getRPGID(const std::string& name)
{
  loadSettings();
  if (myRadarInfos.count(name) > 0) {
    return myRadarInfos[name].rpgid;
  }
  return -1;
}

int
ConfigRadarInfo::getFrequency(const std::string& name)
{
  loadSettings();
  if (myRadarInfos.count(name) > 0) {
    return myRadarInfos[name].frequency;
  }
  return -1;
}

std::string
ConfigRadarInfo::getPolarization(const std::string& name)
{
  loadSettings();
  if (myRadarInfos.count(name) > 0) {
    return myRadarInfos[name].polarization;
  }
  return std::string("u");
}

std::string
ConfigRadarInfo::getFreqBand(const std::string& name)
{
  loadSettings();
  if (myRadarInfos.count(name) > 0) {
    return myRadarInfos[name].freqband;
  }
  return std::string("U");
}

AngleDegs
ConfigRadarInfo::getBeamwidth(const std::string& name)
{
  loadSettings();
  if (myRadarInfos.count(name) > 0) {
    return myRadarInfos[name].beamwidth;
  }
  return -1.0;
}

LLH
ConfigRadarInfo::getLocation(const std::string& name)
{
  loadSettings();
  if (myRadarInfos.count(name) > 0) {
    return myRadarInfos[name].location;
  }
  return LLH();
}

std::string
ConfigRadarInfo::getMagneticOffset(const std::string& name)
{
  loadSettings();
  if (myRadarInfos.count(name) > 0) {
    return myRadarInfos[name].magneticOffset;
  }
  return std::string("");
}
