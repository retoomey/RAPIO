#pragma once

#include <rNamedAny.h>

#include <string>
#include <string_view>
#include <memory>

namespace rapio {
/** AttributeDataType stores a generic collection of attributes.
 * These could coorespond to a global attribute list in netcdf.
 * @include rapio_data
 * @brief Collection of attributes or properties
 * */
class AttributeDataType {
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
  Clone() const;

  /** Set a single-valued attribute.
   *
   * These are the -unit -value pairs from original WDSS2 netcdf
   * files.
   */
  void
  setDataAttributeValue(
    std::string_view key,
    std::string_view value,
    std::string_view unit = "dimensionless"
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
  getString(std::string_view name, std::string& out) const
  {
    if (myAttributes->getString(name, out)) { return true; }
    return myAttributes->getString(std::string(name) + "-value", out);
  }

  /** Set a string */
  inline void
  setString(std::string_view name, std::string_view in)
  {
    return myAttributes->setString(name, in);
  }

  /** Get a double, flexible on casting */
  inline bool
  getDouble(std::string_view name, double& out) const
  {
    if (myAttributes->getDouble(name, out)) { return true; }
    return myAttributes->getDouble(std::string(name) + "-value", out);
  }

  /** Set a double */
  inline void
  setDouble(std::string_view name, double in)
  {
    return myAttributes->setDouble(name, in);
  }

  /** Get a float, flexible on casting */
  inline bool
  getFloat(std::string_view name, float& out) const
  {
    if (myAttributes->getFloat(name, out)) { return true; }
    return myAttributes->getFloat(std::string(name) + "-value", out);
  }

  /** Set a float */
  inline void
  setFloat(std::string_view name, float in)
  {
    return myAttributes->setFloat(name, in);
  }

  /** Get a long, flexible on casting */
  inline bool
  getLong(std::string_view name, long& out) const
  {
    if (myAttributes->getLong(name, out)) { return true; }
    return myAttributes->getLong(std::string(name) + "-value", out);
  }

  /** Set a long in global attributes */
  inline void
  setLong(std::string_view name, long in)
  {
    return myAttributes->setLong(name, in);
  }

protected:

  /** Deep copy our fields to a new subclass */
  void
  deep_copy(const std::shared_ptr<AttributeDataType>& n) const;

  /** Global attributes for data type */
  std::shared_ptr<DataAttributeList> myAttributes;
};
}
