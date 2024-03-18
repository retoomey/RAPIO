#pragma once

#include <rData.h>
#include <rNamedAny.h>

#include <string>
#include <memory>

namespace rapio {
typedef NamedAnyList DataAttributeList;

/** AttributeDataType stores a generic collection of attributes.
 * These could coorespond to a global attribute list in netcdf. */
class AttributeDataType : public Data {
public:

  /** Create empty AttributeDataType */
  AttributeDataType();

  /** Destroy a AttributeDataType */
  virtual ~AttributeDataType(){ }

  /** Set a single-valued attribute.
   *
   * These are the -unit -value pairs from original WDSS2 netcdf
   * files.
   */
  void
  setDataAttributeValue(
    const std::string& key,
    const std::string& value,
    const std::string& unit = "dimensionless"
  );

  /** Global attributes, used by reader/writers */
  std::shared_ptr<DataAttributeList>
  getGlobalAttributes(){ return myAttributes; }

  /** Get a string from global attributes */
  inline bool
  getString(const std::string& name, std::string& out) const
  {
    auto s = myAttributes->get<std::string>(name);

    if (s) {  out = *s; return true; }
    return false;
  }

  /** Set a string in global attributes */
  inline void
  setString(const std::string& name, const std::string& in)
  {
    myAttributes->put<std::string>(name, in);
  }

  /** Get a double from global attributes, flexible on casting */
  inline bool
  getDouble(const std::string& name, double& out) const
  {
    // FIXME: might push down.  With anys we have to
    // explicitly cast things which is currently messy.
    // I'm not too worried if we hide it internally
    auto d = myAttributes->get<double>(name);

    if (d) { out = *d; return true; }
    // Cast up from float.  Could warn here
    auto f = myAttributes->get<float>(name);

    if (f) { out = *f; return true; }
    return false;
  }

  /** Set a double in global attributes */
  inline void
  setDouble(const std::string& name, double in)
  {
    myAttributes->put<double>(name, in);
  }

  /** Get a float from global attributes, flexible on casting */
  inline bool
  getFloat(const std::string& name, float& out) const
  {
    auto f = myAttributes->get<float>(name);

    if (f) { out = *f; return true; }
    auto d = myAttributes->get<double>(name);

    if (d) { out = *d; return true; }
    return false;
  }

  /** Set a float in global attributes */
  inline void
  setFloat(const std::string& name, float in)
  {
    myAttributes->put<float>(name, in);
  }

  /** Get a long from global attributes, flexible on casting */
  inline bool
  getLong(const std::string& name, long& out) const
  {
    auto l = myAttributes->get<long>(name);

    if (l) { out = *l; return true; }
    auto i = myAttributes->get<int>(name); // allow int to long?

    if (i) { out = *i; return true; }
    return false;
  }

  /** Set a long in global attributes */
  inline void
  setLong(const std::string& name, long in)
  {
    myAttributes->put<long>(name, in);
  }

protected:

  /** Global attributes for data type */
  std::shared_ptr<DataAttributeList> myAttributes;
};
}
