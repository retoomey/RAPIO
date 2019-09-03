#pragma once

#include <rData.h>
#include <rConstants.h>
#include <rError.h>

#include <map>
#include <string>
#include <memory>

namespace rapio {
class Time;
class TimeDuration;
class LLH;

/** Simple class for storing name n-value pairs for a datatype */
class DataAttribute : public Data {
public:

  DataAttribute(const std::string& value, const std::string& unit)
    : myValue(value), myUnit(unit){ }

  DataAttribute(){ }

  const std::string
  getValue() const
  {
    return (myValue);
  }

  void
  setValue(const std::string& value)
  {
    myValue = value;
  }

  const std::string
  getUnit() const
  {
    return (myUnit);
  }

  void
  setUnit(const std::string& unit)
  {
    myUnit = unit;
  }

private:

  std::string myValue;
  std::string myUnit;
};

/** The abstract base class for all of the data objects.  */
class DataType : public Data {
public:

  /** key-value pairs that describe the data. */
  typedef  std::map<std::string, DataAttribute> DataAttributes;

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

  // ATTRIBUTE STUFF -----------------------------------------------------------
  std::string
  getDataAttributeString(const std::string& key) const;

  bool
  getRawDataAttribute(const std::string& key,
    std::string                        & name,
    std::string                        & value) const;

  /** Get attribute as float, attempt to convert to different unit if given */
  bool
  getDataAttributeFloat(
    const std::string& key,
    float            & value,
    const std::string& tounit = "") const;

  /** Set a single-valued attribute.
   *
   *  This is a convenience function that allows you to set a
   *  text string as the value of an attribute.
   *
   *  If the attribute key does not already exist, then it will be
   *  constructed.  The empty string may NOT be used as a value for
   *  the attribute; this function will return immediately if the
   *  value is the empty string.
   *
   *  @see setAttribute() to set multiple values for an attribute.
   */
  void
  setDataAttributeValue(
    const std::string& key,
    const std::string& value,
    const std::string& unit = "dimensionless"
  );

  void
  clearAttribute(const std::string& key);

  /** Get the entire map of attributes available for this data type.  */
  const DataAttributes&
  getAttributes2() const
  {
    return (myAttributes2);
  }

  /** Get the units for this datatype */
  virtual std::string
  getUnits();

  // END ATTRIBUTE STUFF
  // -----------------------------------------------------------

  /** Return Location that corresponds to this DataType */
  virtual LLH
  getLocation() const = 0;

  /** Return Time that corresponds to this DataType */
  virtual Time
  getTime() const = 0;

  /** Return 15 minutes as the length of time for which this DataType
   *  is valid.
   *
   *  Override this function in the descendant of DataType as
   *  appropriate.
   */
  virtual TimeDuration
  getExpiryInterval() const;

  /** Return the FilenameDateTime attribute, if set, formatted as:
   *  YYYYMMDD-HHMMSS
   *
   *  String is in UTC.
   *
   *  Override this function in the descendant of DataType as
   *  appropriate.
   */
  virtual Time
  getFilenameDateTime() const;

  /** Set the FilenameDateTime attribute, formatted as:
   *  YYYYMMDD-HHMMSS
   *
   *  String is in UTC.
   *
   *  Override this function in the descendant of DataType as
   *  appropriate.
   */
  virtual void
  setFilenameDateTime(Time aTime);

  /** Initialize TypeName string to invalid value of "not set". */
  DataType();

  /** Call descendant's destructor. */
  virtual ~DataType(){ }

  // QUALITY STUFF -------------------------------------------------------------
  // FIXME: This is just a hack for multi-band.  We're gonna add that to our
  // datatypes
  bool
  hasQuality() const
  {
    return (myQuality != 0);
  }

  const DataType&
  getQuality() const
  {
    return (*myQuality);
  }

  const std::shared_ptr<DataType>
  getQualityPtr() const
  {
    return (myQuality);
  }

  void
  setQuality(std::shared_ptr<DataType> dt)
  {
    myQuality = dt;
  }

  // END QUALITY STUFF ---------------------------------------------------------

  /** 2D write function.  Do we subclass 2D datatypes? */
  virtual void
  set(size_t i, size_t j, const float& v){ }

  /** Fill datatype up with a value. */
  virtual void
  fill(const float& val){ }

protected:

  /** String used for writer/reader factory, such as 'RadialSet' */
  std::string myDataType;

  /** The TypeName of the data contained.  */
  std::string myTypeName;

  /** Attribute storage */
  DataAttributes myAttributes2;

  /** Old hack I think.  We need to remove or rethink this design */
  std::shared_ptr<DataType> myQuality;
};
}
