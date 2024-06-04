#pragma once

#include <rData.h>
#include <rNamedAny.h>

#include <string>
#include <memory>

namespace rapio {
/** AttributeDataType stores a generic collection of attributes.
 * These could coorespond to a global attribute list in netcdf. */
class AttributeDataType : public Data {
public:

  /** Create empty AttributeDataType */
  AttributeDataType();

  /** Public API for users to create an AttributeDataType */
  static std::shared_ptr<AttributeDataType>
  Create()
  {
    auto nsp = std::make_shared<AttributeDataType>();

    return nsp;
  }

  /** Destroy an AttributeDataType */
  virtual ~AttributeDataType(){ }

  /** Public API for users to clone an AttributeDataType */
  std::shared_ptr<AttributeDataType>
  Clone();

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

  // ----------------------------------------------
  // Convenience routines for common types
  // Debating is vs have here, though these are
  // double inlined so the cost should be nothing here.
  // These coorespond to the netcdf global attributes,
  // for the local attributes on an array, @see DataArray

  /** Get a string */
  inline bool
  getString(const std::string& name, std::string& out) const
  {
    return myAttributes->getString(name, out);
  }

  /** Set a string */
  inline void
  setString(const std::string& name, const std::string& in)
  {
    return myAttributes->setString(name, in);
  }

  /** Get a double, flexible on casting */
  inline bool
  getDouble(const std::string& name, double& out) const
  {
    return myAttributes->getDouble(name, out);
  }

  /** Set a double */
  inline void
  setDouble(const std::string& name, double in)
  {
    return myAttributes->setDouble(name, in);
  }

  /** Get a float, flexible on casting */
  inline bool
  getFloat(const std::string& name, float& out) const
  {
    return myAttributes->getFloat(name, out);
  }

  /** Set a float */
  inline void
  setFloat(const std::string& name, float in)
  {
    return myAttributes->setFloat(name, in);
  }

  /** Get a long, flexible on casting */
  inline bool
  getLong(const std::string& name, long& out) const
  {
    return myAttributes->getLong(name, out);
  }

  /** Set a long in global attributes */
  inline void
  setLong(const std::string& name, long in)
  {
    return myAttributes->setLong(name, in);
  }

protected:

  /** Deep copy our fields to a new subclass */
  void
  deep_copy(std::shared_ptr<AttributeDataType> n);

  /** Global attributes for data type */
  std::shared_ptr<DataAttributeList> myAttributes;
};
}
