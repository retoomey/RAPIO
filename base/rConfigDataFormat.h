#pragma once

#include <rIO.h>
#include <rConfig.h>

#include <string>
#include <memory>
#include <map>

namespace rapio {
class IODataType;
class DataType;
class IndexType;

/** Class storing lookup settings for a particular data format */
class DataFormatSetting : public IO {
public:
  /** Datatype key these write settings are for */
  std::string datatype;

  bool compress; // Wondering if these can just be 'general'
  bool subdirs;
  bool cdmcompliance;
  bool faacompliance;
  /** Factory key for IODataType reader/writer */
  std::string format;

  /** Other general attributes */
  std::map<std::string, std::string> attributes;
};

class ConfigDataFormat : public ConfigType {
public:
  ConfigDataFormat();

  /** Get a setting based on datatype */
  static std::shared_ptr<DataFormatSetting>
  getSetting(const std::string& datatype);

private: // Config called

  /** Called by config during startup */
  virtual bool
  readConfig(){ return readInSettings(); }

  /** Add a failsafe default writer configuration for any error */
  static void
  addFailsafe();

  /** Read in all settings */
  static bool
  readInSettings();

  /** Store collection of configurations for different data formats */
  static std::vector<std::shared_ptr<DataFormatSetting> > mySettings;
};
}
