#pragma once

#include <rIO.h>
#include <rIOURL.h>

#include <memory>

namespace rapio {
/** Base class for reading/writing parameter configuration class.
 * This is a helper class for RAPIOOptions.
 * xml currently refers to the wdssii old style .xml config file passed to
 * algorithms.
 * config currently refers to the hmet .config style file passed to hmet
 * algorithms.
 * @author Robert Toomey
 */
class AlgConfigFile : public IO {
public:

  // Factory methods --------------------------------------
  //

  /** Introduce base algorithm config classes on startup */
  static void
  introduceSelf();

  /** Introduce subclass into factories */
  static void
  introduce(const std::string      & protocol,
    std::shared_ptr<AlgConfigFile> factory);

  /** Return an algorthm config file for given URL */
  static std::shared_ptr<AlgConfigFile>
  getConfigFileForURL(const URL& path);

  /** Return an algorithm config file reader/writer for given type */
  static std::shared_ptr<AlgConfigFile>
  getConfigFile(const std::string& type);

  /** Callback for reading a configuration at given URL location */
  virtual bool
  readConfigURL(const URL   & path,
    std::vector<std::string>& options, // deliberately avoiding maps
    std::vector<std::string>& values) = 0;

  /** Callback for writing a configuration at given URL location */
  virtual bool
  writeConfigURL(const URL  & path,
    const std::string       & program,
    std::vector<std::string>& options, // deliberately avoiding maps
    std::vector<std::string>& values) = 0;
};

/** Can read write a xml style file.  This matches the oldstyle
 * xml file we use in WDSS2 */
class AlgXMLConfigFile : public AlgConfigFile
{
public:
  /** Introduce base algorithm config classes on startup */
  static void
  introduceSelf();

  /** Callback for reading a configuration at given URL location */
  virtual bool
  readConfigURL(const URL   & path,
    std::vector<std::string>& options, // deliberately avoiding maps
    std::vector<std::string>& values) override;

  /** Callback for writing a configuration at given URL location */
  virtual bool
  writeConfigURL(const URL  & path,
    const std::string       & program,
    std::vector<std::string>& options, // deliberately avoiding maps
    std::vector<std::string>& values) override;
};

/** Can read write a flat config file.  This matches the oldstyle
 * HMET file. */
class AlgFLATConfigFile : public AlgConfigFile
{
public:
  /** Introduce base algorithm config classes on startup */
  static void
  introduceSelf();

  /** Callback for reading a configuration at given URL location */
  virtual bool
  readConfigURL(const URL   & path,
    std::vector<std::string>& options, // deliberately avoiding maps
    std::vector<std::string>& values) override;

  /** Callback for writing a configuration at given URL location */
  virtual bool
  writeConfigURL(const URL  & path,
    const std::string       & program,
    std::vector<std::string>& options, // deliberately avoiding maps
    std::vector<std::string>& values) override;
};
}
