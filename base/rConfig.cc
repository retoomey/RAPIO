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
#include "rConfigIODataType.h"
#include "rConfigLogging.h"

#include <set>
#include <cstdlib>  // getenv()
#include <unistd.h> // access()
#include <iostream>

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
      LogSevere("WEB READ?\n");
    }
    testMe.setPath(p);
  }

  URL ret;
  if (found) {
    ret = testMe;
    LogDebug("Read: " << testMe.getPath() << "\n");
  } else {
    LogDebug("WARNING! " << relativePath << " was not found.\n");
  }

  return (ret);
} // Config::getAbsoluteForRelative

bool
Config::addSearchPath(const URL& absolutePath)
{
  const bool exists = absolutePath.isLocal() &&
    OS::isDirectory(absolutePath.getPath());

  if (exists) {
    // This will clean up the path
    std::string canon = OS::canonical(absolutePath.toString());
    // Make sure there's a '/' on the end of every URL, for when we add relatives.
    if (!Strings::endsWith(canon, "/")) { canon += "/"; }
    mySearchPaths.push_back(canon);
    // We print them all out later anyway
    // LogInfo("Adding search path \"" << canon << "\"\n");
  }
  return exists;
}

void
Config::addSearchFromString(const std::string& pathgroup)
{
  std::vector<std::string> paths;
  Strings::splitWithoutEnds(pathgroup, ':', &paths);
  for (auto& it:paths) {
    addSearchPath(URL(it));                  // Direct path maybe given
    addSearchPath(URL(it + "/RAPIOConfig")); // RAPIO folder...
    addSearchPath(URL(it + "/w2config"));    // W2 folder...
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
  // Look for global configuration file folder.

  // First priority is environment variables
  const std::string rapio = enforceLastSlash(getEnvVar("RAPIO_CONFIG_LOCATION"));
  const std::string w2    = enforceLastSlash(getEnvVar("W2_CONFIG_LOCATION"));

  addSearchFromString(rapio);
  addSearchFromString(w2);

  // Fall back to relative from binary when running example program
  auto path = OS::getProcessPath() + "/../../RAPIOConfig/";
  addSearchFromString(path);
  path = OS::getProcessPath() + "/../RAPIO/RAPIOConfig/"; // From bin folder to source
  addSearchFromString(path);

  // Fall back to home directory
  if (mySearchPaths.size() < 1) {
    const std::string home(getEnvVar("HOME"));
    if (OS::isDirectory(home)) {
      addSearchFromString(home);
    }
  }
  if (mySearchPaths.size() < 1) {
    LogSevere(
      "No valid global configuration path found in environment variables RAPIO_CONFIG_LOCATION, W2_CONFIG_LOCATION or home directory.\n");
    return false;
  }

  /*std::string s;
   * for (auto& it:mySearchPaths) {
   * s += "\n\t[" + it.toString() + "]";
   * }
   * LogDebug("Global configuration search order:" << s << '\n');
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
    LogSevere("Failed to find/read global configuration file: " << file << "\n");
    LogSevere("Global configuration search order:" << s << '\n');
  } else {
    LogDebug("Global configuration search order:" << s << '\n');
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
    LogSevere("Failed to read initial configurations due to missing or bad file format.\n");
    LogSevere(
      "Set environment variable RAPIO_CONFIG_LOCATION or W2_CONFIG_LOCATION to help RAPIO find your configuration files.\n");
  }
  return success;
} // Config::initialize

std::string
Config::getEnvVar(const std::string& name)
{
  std::string ret;
  const char * envPath = ::getenv(name.c_str());

  if (envPath) {
    ret = envPath;
  } else {
    ret = "";
  }
  return (ret);
}

void
Config::setEnvVar(const std::string& envVarName, const std::string& value)
{
  setenv(envVarName.c_str(), value.c_str(), 1);
  LogDebug(envVarName << " = " << value << "\n");
}

std::shared_ptr<PTreeData>
Config::huntXML(const std::string& pathName)
{
  // FIXME: assuming always relative path?  Should
  // we just hide it all from caller?
  const URL loc(getAbsoluteForRelative(pathName));

  if (!loc.empty()) {
    return (IODataType::read<PTreeData>(loc.toString(), "xml"));
  }
  return (nullptr);
}
