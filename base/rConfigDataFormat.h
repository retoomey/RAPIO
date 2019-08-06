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
  bool compress;
  bool subdirs;
  std::string datatype;
  std::shared_ptr<IODataType> prototype;
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

  /** Read in all settings */
  static bool
  readInSettings();

  /** Store collection of configurations for different data formats */
  static std::map<std::string, std::shared_ptr<DataFormatSetting> > mySettings;
};
}
