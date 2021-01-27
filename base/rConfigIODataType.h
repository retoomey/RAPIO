#pragma once

#include <rConfig.h>

#include <map>
#include <string>

namespace rapio {
/** Holds loadable configuration settings for IODataType */
class ConfigIODataType : public ConfigType {
public:
  /** Virtual object from config map to static for clarity. */
  virtual bool
  readConfig(std::shared_ptr<PTreeData> d) override { return readSettings(d); }

  /** Introduce self to configuration */
  static void
  introduceSelf();

  /** Actual work of reading/checking settings */
  static bool
  readSettings(std::shared_ptr<PTreeData> );

  /** Do we output with subdirs or combined file name? */
  static bool
  getUseSubDirs();

  /** Read NODE of settings for a key */
  static std::shared_ptr<PTreeNode>
  getSettings(const std::string& key);

private:
  /** Do we output with subdirs or combined file name? */
  static bool myUseSubDirs;

  /** Database of keys to settings */
  static std::map<std::string, std::shared_ptr<PTreeNode> > myDatabase;
};
}
