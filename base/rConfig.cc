#include "rConfig.h"

#include "rConstants.h"
#include "rStrings.h"
#include "rError.h"
#include "rOS.h"
#include "rFactory.h"
#include "rIODataType.h"

// ConfigTypes
#include "rConfigDirectoryMapping.h"
#include "rConfigDataFormat.h"

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
    LogInfo(">>>Read:" << testMe.getPath() << "\n");
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

  // Make sure there's a '/' on the end of every URL, for when we add relatives.
  std::string p = absolutePath.toString();
  if (!Strings::endsWith(p, "/")) { p += "/"; }

  if (exists) {
    mySearchPaths.push_back(absolutePath);
    LogInfo("Adding search path \"" << absolutePath << "\"\n");
  }
  return exists;
}

void
Config::addSearchFromString(const std::string& pathgroup)
{
  std::vector<std::string> paths;
  Strings::splitWithoutEnds(pathgroup, ':', &paths);
  for (auto& it:paths) {
    addSearchPath(URL(it));               // Direct path maybe given
    addSearchPath(URL(it + "/rconfig"));  // RAPIO folder...
    addSearchPath(URL(it + "/w2config")); // W2 folder...
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
  // I think we should read the basic configuration files at start up and
  // store variables in us.

  // Add our configuration subclasses
  // for the moment hard-coded.  I'll probably let algorithms
  // declare new XML configuration classes pre-startup
  std::shared_ptr<ConfigType> cdm = std::make_shared<ConfigDirectoryMapping>();
  introduce("directorymap", cdm);
  std::shared_ptr<ConfigType> df = std::make_shared<ConfigDataFormat>();
  introduce("dataformat", df);
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

void
Config::initialize()
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

  // Fall back to home directory
  if (mySearchPaths.size() < 1) {
    const std::string home(getEnvVar("HOME"));
    // if (OS::testFile(home, OS::FILE_IS_DIRECTORY)) {
    if (OS::isDirectory(home)) {
      addSearchFromString(home);
    }
  }
  if (mySearchPaths.size() < 1) {
    LogSevere(
      "No valid global configuration path found in environment variables RAPIO_CONFIG_LOCATION, W2_CONFIG_LOCATION or home directory.\n");
    exit(1);
  }
  std::string s;
  for (auto& it:mySearchPaths) {
    s += "\n\t[" + it.toString() + "]";
  }
  LogInfo("Global configuration search order:" << s << '\n');

  // Read all initial startup configurations
  // FIXME: Allow fail here optionally
  auto configs = Factory<ConfigType>::getAll();
  bool success = true;
  for (auto& i:configs) {
    success &= i.second->readConfig();
  }
  if (success == false) {
    LogSevere(
      "Failed to find/load required global configuration files, you might need to set environment variable RAPIO_CONFIG_LOCATION or W2_CONFIG_LOCATION\n");
    exit(1);
  }
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
  LogInfo("Set environment: " << envVarName << " = " << value << "\n");
}

std::shared_ptr<XMLData>
Config::huntXML(const std::string& pathName)
{
  // FIXME: assuming always relative path?  Should
  // we just hide it all from caller?
  const URL loc(getAbsoluteForRelative(pathName));

  if (!loc.empty()) {
    return (IODataType::read<XMLData>(loc, "xml"));
  }
  return (nullptr);
}
