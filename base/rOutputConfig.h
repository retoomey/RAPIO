#pragma once

namespace rapio {
/** An API for specifying how to output a DataType.
 * Default system flags and abilities will become API get/set
 * in order to avoid magic strings and mistakes/unknown
 * general abilities.
 *
 * Modules will still have the ability to set special
 * flags by string. Recommend constants set in the modules,
 * for example netcdf flags from rapiosettings.xml.
 *
 * @author Robert Toomey
 */
class OutputConfig {
public:
  OutputConfig() = default;

  /** Set a special string by key.  Will be used by dynamic modules
   * to store non-standard custom values */
  OutputConfig&
  set(const std::string& key, const std::string& value)
  {
    myParams[key] = value;
    return *this;
  }

  /** Check if a key exists */
  bool
  has(const std::string& key) const
  {
    return myParams.find(key) != myParams.end();
  }

  /** Get a key with a safe default */
  std::string
  get(const std::string& key, const std::string& defaultValue = "") const
  {
    auto it = myParams.find(key);

    if (it != myParams.end()) {
      return it->second;
    }
    return defaultValue;
  }

  /** Return map for backward compatibility for now */
  const std::map<std::string, std::string>&
  toMap() const
  {
    return myParams;
  }

  /** Temp legacy code compatibility if needed */
  std::map<std::string, std::string>&
  getMapRef()
  {
    return myParams;
  }

private:

  /** Generic storage */
  std::map<std::string, std::string> myParams;
};
}
