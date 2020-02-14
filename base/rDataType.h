#pragma once

#include <rData.h>
#include <rConstants.h>
#include <rError.h>
#include <rNamedAny.h>

#include <map>
#include <string>
#include <memory>

#include <boost/any.hpp>
#include <boost/optional.hpp>

namespace rapio {
class Time;
class TimeDuration;
class LLH;

typedef NamedAnyList DataAttributeList;

/** The abstract base class for all of the data objects.  */
class DataType : public Data {
public:

  /** Format a subtype string utility function */
  std::string
  formatString(float spec,
    size_t           tot,
    size_t           prec) const;

  /** Get the data type we store */
  std::string
  getDataType() const
  {
    return (myDataType);
  }

  /** Set the data type we store */
  void
  setDataType(const std::string& d)
  {
    myDataType = d;
  }

  /** Get subtype for writing */
  bool
  getSubType(std::string& result) const;

  /** Return the TypeName of this DataType. */
  const std::string &
  getTypeName() const;

  /** Set the TypeName of this DataType. */
  void
  setTypeName(const std::string&);

  /** Generated subtype for outputting from empty */
  virtual std::string
  getGeneratedSubtype() const
  {
    return ("");
  }

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

  /** Sync any internal stuff to data from current attribute list,
   * return false on fail. */
  virtual bool
  initFromGlobalAttributes();

  /** Make sure global attribute list reflects any internal variables */
  virtual void
  updateGlobalAttributes(const std::string& encoded_type);

  /** Return Location that corresponds to this DataType */
  virtual LLH
  getLocation() const = 0;

  /** Return Time that corresponds to this DataType */
  virtual Time
  getTime() const = 0;

  /** Initialize TypeName string to invalid value of "not set". */
  DataType();

  /** Call descendant's destructor. */
  virtual ~DataType(){ }

  /** Raw attribute pointer, used by reader/writers */
  DataAttributeList *
  getRawGlobalAttributePointer(){ return &myAttributes; }

protected:

  /** Global attributes for data type */
  DataAttributeList myAttributes;

  /** String used for writer/reader factory, such as 'RadialSet' */
  std::string myDataType;

  /** The TypeName of the data contained.  */
  std::string myTypeName;
};
}
