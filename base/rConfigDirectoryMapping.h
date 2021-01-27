#pragma once

#include <rConfig.h>

#include <map>
#include <string>

namespace rapio {
/** Translates paths from one setting to another globally
 * using an xml file */
class ConfigDirectoryMapping : public ConfigType {
public:

  /** Translate strings from one directory to another */
  static void
  doDirectoryMapping(std::string& sub_me);

  /** Virtual object from config map to static for clarity. */
  virtual bool
  readConfig(std::shared_ptr<PTreeData> ) override { return readInSettings(); }

  /** Introduce self to configuration */
  static void
  introduceSelf();

  /** Actual work of reading/checking settings */
  static bool
  readInSettings();

private:

  /** Mapping of directory string to replacements */
  static std::map<std::string, std::string> myMappings;
};
}
