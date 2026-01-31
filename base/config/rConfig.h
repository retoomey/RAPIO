#pragma once

#include <rURL.h>
#include <rPTreeData.h>

#include <vector>
#include <memory>
#include <string>

namespace rapio {
/** Config types are registered with config, they handle a particular
 * group of configuration
 * @ingroup rapio_io
 * @brief Registered with Config to handle one group of configuration
 **/
class ConfigType {
public:

  /** Do we pre-read configuration on startup?  This is preferred if
   * this configuration has no fall back situation */
  virtual bool
  onStartUp(){ return true; }

  /** Read configuration information from xml/json data. */
  virtual bool
  readConfig(std::shared_ptr<PTreeData> global){ return true; };

  /** Read configuration information from a command line string */
  virtual bool
  readString(const std::string& param){ return true; };
};

/**
 * Find configuration files from user- and system- defined locations.
 *
 * When searching for configuration files, Config will look in:
 * <ol>
 * <li>The $RAPIO_CONFIG_LOCATION and $W2_CONFIG_LOCATION environmental variables
 * </ol>
 * @ingroup rapio_utility
 * @brief Configuration API allowing ConfigType classes
 */
class Config {
public:

  /** Introduce self and default helper classes */
  static void
  introduceSelf();

  /** Initialize configuration by setting up search paths */
  static bool
  initialize();

  /** Initial set up of global configuration search paths */
  static bool
  setUpSearchPaths();

  /** Initial read of global configuration file */
  static std::shared_ptr<PTreeData>
  readGlobalConfigFile();

  /**
   * Returns the absolute location of this file.
   * The first match in our search paths is returned.
   * @return empty url if the file doesn't exist in the search paths.
   */
  static URL
  getConfigFile(const std::string& pathToFile);

  /** Finds the absolute path of the given relative path. */
  static URL
  getAbsoluteForRelative(const std::string& relativePath);

  /* Read from a string, hunt in config */
  static std::shared_ptr<PTreeData>
  huntXML(const std::string& pathname);

  /** Use this to introduce new subclasses. */
  static void
  introduce(const std::string   & name,
    std::shared_ptr<ConfigType> new_subclass);

  /** Virtual destructor. */
  virtual ~Config(){ }

private:

  /** Constructor not implemente, only static functions */
  Config();

  /** Constructor not implemented, only static functions. */
  Config(const Config&);

  /** Copy not implemented, only static functions. */
  Config&
  operator = (const Config&);

  /** Add a directory to the end of the list of search paths */
  static bool
  addSearchPath(const URL&);

  /** Parse a collection of paths separated by : */
  static void
  addSearchFromString(const std::string& pathgroup);

  /** the order of search. The first match wins. */
  static std::vector<URL> mySearchPaths;
};
}
