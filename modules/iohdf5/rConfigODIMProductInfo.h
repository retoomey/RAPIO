#pragma once

#include <rConfig.h>

namespace rapio {
/** Handles storage of information about prroduceODIM data
 *
 * @author Michael Taylor
 *    Original design
 *
 * @auther Robert Toomey
 *   Made a ConfigType.  Maybe at some point we'd want it
 *   as a general startup configuration check.
 *
 * @see ConfigRadarInfo
 */
class ODIMProductInfo {
  friend class ConfigODIMProductInfo;
public:

  /** Construct a default odim product info */
  ODIMProductInfo() :
    odimProductName(""),
    mrmsProductName("Unknown"),
    unit("dimensionless"), // FIXME: Dimensionless maybe?
    colorMap("Unknown"){ }

  /** Construct a ODIM product info */
  ODIMProductInfo(const std::string& odimProductNameIn,
    const std::string              & mrmsProductNameIn,
    const std::string              & unitIn,
    const std::string              colorMapIn) :
    odimProductName(odimProductNameIn),
    mrmsProductName(mrmsProductNameIn),
    unit(unitIn),
    colorMap(colorMapIn){ }

  /** Get the untranslated ODIM data type */
  const std::string& getODIMDataType(){ return odimProductName; }

  /** Get the translated MRMS data type */
  const std::string& getMRMSDataType(){ return mrmsProductName; }

  /** Get the MRMS units */
  const std::string& getUnits(){ return unit; }

  /** Get the MRMS color map */
  const std::string& getColorMap(){ return colorMap; }

protected:
  std::string odimProductName;
  std::string mrmsProductName;
  std::string unit;
  std::string colorMap;
};

class ConfigODIMProductInfo : public ConfigType {
public:

  // Stubs for ConfigType, though we don't load on startup, but
  // on initializating of the hdf5 module instead.

  /** Virtual object from config map to static for clarity. */
  virtual bool
  readConfig(std::shared_ptr<PTreeData> d) override { return readSettings(d); }

  /** Actual work of reading/checking settings */
  static bool
  readSettings(std::shared_ptr<PTreeData> d){ return true; }

  /** Check and load settings */
  static void
  loadSettings();

  /** Get a product info */
  static ODIMProductInfo
  getProductInfo(const std::string& name);

private:

  /** Path for our configuration file */
  static const std::string ConfigODIMProductInfoXML;

  /** Have we read settings yet? */
  static bool myReadSettings;

  /** Map from names to Radar Infos */
  static std::map<std::string, ODIMProductInfo> myProductInfos;
};
}
