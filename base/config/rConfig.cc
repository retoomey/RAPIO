#include "rConfig.h"

#include "rConstants.h"
#include "rStrings.h"
#include "rError.h"
#include "rOS.h"
#include "rFactory.h"
#include "rIODataType.h"

// ConfigTypes
#include "rUnit.h"
#include "rConfigDirectoryMapping.h"
#include "rConfigRadarInfo.h"
#include "rConfigIODataType.h"
#include "rConfigLogging.h"

#include <set>
#include <cstdlib>  // getenv()
#include <unistd.h> // access()
#include <iostream>

#include "config.h"

using namespace rapio;
using namespace std;

std::vector<URL> Config::mySearchPaths;

void
Config::introduce(const std::string & name,
  std::shared_ptr<ConfigType>       new_subclass)
{
  Factory<ConfigType>::introduce(name, new_subclass);
}

URL
Config::getAbsoluteForRelative(const std::string& relativePath)
{
  URL testMe;
  bool found = false;

  for (auto& i:mySearchPaths) {
    testMe = i;
    std::string p = testMe.getPath();

    // We added a "/" in addSearchPath
    // testMe.path += "/" + relativePath;
    p += relativePath;

    if (testMe.isLocal()) {
      if (!::access(p.c_str(), R_OK)) {
        // Read locally...
        found = true;
        testMe.setPath(p);
        break;
      }
    } else {
      fLogSevere("WEB READ?");
    }
    testMe.setPath(p);
  }

  URL ret;

  if (found) {
    ret = testMe;
    fLogDebug("Read: {}", testMe.getPath());
  } else {
    fLogDebug("WARNING! {} was not found.", relativePath);
  }

  return (ret);
} // Config::getAbsoluteForRelative

bool
Config::addSearchPath(const URL& absolutePath)
{
  if (absolutePath.empty()) { return false; }

  const bool exists = absolutePath.isLocal() &&
    OS::isDirectory(absolutePath.getPath());

  if (exists) {
    // This will clean up the path
    std::string canon = OS::canonical(absolutePath.toString());
    // Make sure there's a '/' on the end of every URL, for when we add relatives.
    if (!Strings::endsWith(canon, "/")) { canon += "/"; }
    mySearchPaths.push_back(canon);
    // We print them all out later anyway
    // fLogInfo("Adding search path \"{}\"", canon);
  }
  return exists;
}

void
Config::addSearchFromString(const std::string& pathgroup)
{
  std::vector<std::string> paths;

  Strings::splitWithoutEnds(pathgroup, ':', &paths);
  for (auto& it:paths) {
    if (Strings::endsWith(it, "RAPIOConfig") ||
      Strings::endsWith(it, "w2config"))
    {
      addSearchPath(URL(it)); // Direct path given
    } else {
      if (!it.empty()) {
        addSearchPath(URL(it + "/RAPIOConfig")); // RAPIO folder...
        addSearchPath(URL(it + "/w2config"));    // W2 folder...
      }
    }
  }
}

URL
Config::getConfigFile(const std::string& l)
{
  bool is_absolute = !l.empty() && l[0] == '/';

  if (is_absolute) { return l; };
  return getAbsoluteForRelative(l);
}

void
Config::introduceSelf()
{
  // Add our default configuration subclasses
  ConfigUnit::introduceSelf();       // udunits
  ConfigIODataType::introduceSelf(); // all iodatatype reader/writer settings
  // FIXME: could become general index settings
  ConfigDirectoryMapping::introduceSelf(); // directory (used by index)
  ConfigRadarInfo::introduceSelf();        // radar info database
  ConfigLogging::introduceSelf();          // logging, colors, etc.
}

namespace {
// Simple add / to directory if not there
std::string
enforceLastSlash(const std::string& str)
{
  auto out = str;

  if (str.length() > 0) {
    if (str.back() != '/') {
      out.push_back('/');
    }
  }
  return out;
}
}

bool
Config::setUpSearchPaths()
{
  // First priority is environment variables
  const std::string rapio = enforceLastSlash(OS::getEnvVar("RAPIO_CONFIG_LOCATION"));
  const std::string w2    = enforceLastSlash(OS::getEnvVar("W2_CONFIG_LOCATION"));

  addSearchFromString(rapio);
  addSearchFromString(w2);

  // Fall back to the compiled source RAPIOConfig location
  std::string compilePath(SOURCE_PATH);

  addSearchPath(compilePath + "/RAPIOConfig");

  // Fall back to home directory
  const std::string home(OS::getEnvVar("HOME"));

  addSearchPath(home);

  if (mySearchPaths.size() < 1) {
    fLogSevere(
      "No valid global configuration path found in environment variables RAPIO_CONFIG_LOCATION, W2_CONFIG_LOCATION or home directory.");
    return false;
  }

  /*std::string s;
   * for (auto& it:mySearchPaths) {
   * s += "\n\t[" + it.toString() + "]";
   * }
   * fLogDebug("Global configuration search order:{}", s);
   */
  return true;
} // Config::setUpSearchPaths

std::shared_ptr<PTreeData>
Config::readGlobalConfigFile()
{
  const std::string file = "rapiosettings.xml";
  auto doc = huntXML(file); // or maybe json

  std::string s;

  for (auto& it:mySearchPaths) {
    s += "\n\t[" + it.toString() + "]";
  }
  if (doc == nullptr) {
    fLogSevere("Failed to find/read global configuration file: {}", file);
    fLogSevere("Global configuration search order:{}", s);
  } else {
    fLogDebug("Global configuration search order:{}", s);
  }
  return doc;
}

bool
Config::initialize()
{
  // Set up all search files to configuration files
  bool success = false;

  if (setUpSearchPaths()) {
    // Attempt to read the required global configuration file
    auto doc = readGlobalConfigFile();
    if (doc != nullptr) {
      // Now let registered classes search for either nodes in the
      // global file, or individual files...
      auto configs = Factory<ConfigType>::getAll();
      success = true;
      for (auto& i:configs) {
        if (i.first != "logging") {
          success &= i.second->readConfig(doc);
        }
      }
      // Do logging after the others, let them all use the built in log settings
      for (auto& i:configs) {
        if (i.first == "logging") {
          success &= i.second->readConfig(doc);
          break;
        }
      }
    }
  }

  if (success == false) {
    fLogSevere("Failed to read initial configurations due to missing or bad file format.");
    fLogSevere(
      "Set environment variable RAPIO_CONFIG_LOCATION or W2_CONFIG_LOCATION to help RAPIO find your configuration files.");
  }
  return success;
} // Config::initialize

std::shared_ptr<PTreeData>
Config::huntXML(const std::string& pathName)
{
  const URL loc(getAbsoluteForRelative(pathName));

  if (!loc.empty()) {
    return (IODataType::read<PTreeData>(loc.toString(), "xml"));
  }
  return (nullptr);
}
