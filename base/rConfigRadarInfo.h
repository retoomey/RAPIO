#pragma once

#include <rConfig.h>

#include <string>

namespace rapio {
/** Holder for a RadarInfo entry */
class RadarInfo {
  friend class ConfigRadarInfo;
public:

  /** STL Construct a RadarInfo */
  RadarInfo(){ }

  /** Construct a RadarInfo */
  RadarInfo(
    const std::string& aName,
    const std::string& aSite,
    int              aRpgid,
    int              aFrequency,
    const std::string& aPolarization,
    const std::string& aFreqband,
    AngleDegs        aBeamwidth,
    const LLH        aLLH,
    const std::string& magnetic) : name(aName), site(aSite), rpgid(aRpgid), frequency(aFrequency),
    polarization(aPolarization), freqband(aFreqband), beamwidth(aBeamwidth), location(aLLH), magneticOffset(magnetic){ }

  /** Get name of this info */
  std::string
  getName() const { return name; }

  /** Get site */
  std::string
  getSite() const { return site; }

  /** Get RPGid */
  int
  getRPGID() const { return rpgid; }

  /** Get frequency */
  int
  getFrequency() const { return frequency; }

  /** Get polarization, such as d for dualpol, s for single, etc. */
  std::string
  getPolarization() const { return polarization; }

  /** Get frequency band such as C, S, X, or U for unknown */
  std::string
  getFreqBand() const { return freqband; }

  /** Get beamwidth */
  AngleDegs
  getBeamwidth() const { return beamwidth; }

  /** Get location */
  LLH
  getLocation() const { return location; }

  /** Get magnetic offset.  FIXME: could add all the stuff Shawn did for
   * getting the angle out. */
  std::string
  getMagneticOffset() const { return magneticOffset; }

private:

  std::string name;
  std::string site;
  int rpgid;
  int frequency;
  std::string polarization; // d dualpol, s single, u unknown
  std::string freqband;     // C, S, X, U
  AngleDegs beamwidth;
  LLH location;
  std::string magneticOffset; // 10W, etc. for TDWR radars
};

/** Reads a static database for storing radar information */
class ConfigRadarInfo : public ConfigType {
public:

  // Public access methods

  /** Call this to free the database RAM if no longer needed,
   * the database will be reread if called again */
  static void
  releaseDatabase()
  {
    myRadarInfos.clear();
    myReadSettings = false;
  }

  /** Get RadarInfo given a radar name. Can also use direct but slower methods below if
   * only needing say one field of the info. */
  static RadarInfo
  getRadarInfo(const std::string& name);

  /** Get site given a radar name */
  static std::string
  getSite(const std::string& name);

  /** Get RPGid given a radar name */
  static int
  getRPGID(const std::string& name);

  /** Get frequency given a radar name */
  static int
  getFrequency(const std::string& name);

  /** Get polarization given a radar name, such as d for dualpol, s for single, etc. */
  static std::string
  getPolarization(const std::string& name);

  /** Get frequency band such as C, S, X, or U for unknown */
  static std::string
  getFreqBand(const std::string& name);

  /** Get beamwidth */
  static AngleDegs
  getBeamwidth(const std::string& name);

  /** Get location given a radar name */
  static LLH
  getLocation(const std::string& name);

  /** Get magnetic offset */
  static std::string
  getMagneticOffset(const std::string& name);

  // End access methods

  /** Path for our configuration file */
  static const std::string ConfigRadarInfoXML;

  /** Virtual object from config map to static for clarity. */
  virtual bool
  readConfig(std::shared_ptr<PTreeData> d) override { return readSettings(d); }

  /** Introduce self to configuration */
  static void
  introduceSelf();

  /** Actual work of reading/checking settings */
  static bool
  readSettings(std::shared_ptr<PTreeData> d);

  /** Check and load settings */
  static void
  loadSettings();

private:

  /** Have we read settings yet? */
  static bool myReadSettings;

  /** Map from names to Radar Infos */
  static std::map<std::string, RadarInfo> myRadarInfos;
};
}
