#pragma once
#include <rURL.h>

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
class IOConfig {
public:
  IOConfig() = default;

  /** Return command line parameter string as a URL */
  URL
  getParamURL()
  {
    return URL(myParams);
  }

  void
  setParams(const std::string& p)
  {
    myParams = p;
  }

  /** Set a special string by key.  Will be used by dynamic modules
   * to store non-standard custom values */
  IOConfig&
  set(const std::string& key, const std::string& value)
  {
    myLookup[key] = value;
    return *this;
  }

  /** Check if a key exists */
  bool
  has(const std::string& key) const
  {
    return myLookup.find(key) != myLookup.end();
  }

  /** Get a key with a safe default */
  std::string
  get(const std::string& key, const std::string& defaultValue = "") const
  {
    auto it = myLookup.find(key);

    if (it != myLookup.end()) {
      return it->second;
    }
    return defaultValue;
  }

  /** Return map for backward compatibility for now */
  const std::map<std::string, std::string>&
  toMap() const
  {
    return myLookup;
  }

  /** Temp legacy code compatibility if needed */
  std::map<std::string, std::string>&
  getMapRef()
  {
    return myLookup;
  }

  /** Temp legacy code hack */
  void
  setMap(std::map<std::string, std::string>& in)
  {
    myLookup = in;
  }

private:

  /** Params from command line */
  std::string myParams;

  /** Generic storage */
  std::map<std::string, std::string> myLookup;
};
}
