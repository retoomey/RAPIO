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

private: // Config read

  virtual bool
  readConfig() override { return readInSettings(); }

  static bool
  readInSettings();

  static std::map<std::string, std::string> myMappings;
};
}
