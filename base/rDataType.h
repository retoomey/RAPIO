#pragma once

#include <rData.h>
#include <rConstants.h>
#include <rError.h>

#include <map>
#include <string>
#include <memory>

#include <boost/any.hpp>
#include <boost/optional.hpp>

namespace rapio {
class Time;
class TimeDuration;
class LLH;

/** Store a name to std::shared_ptr of anything pair.
 * FIXME: own file at some point */
class NamedAny : public Data
{
public:
  /** Create a named object */
  NamedAny(const std::string& name) : myName(name)
  { }

  /** Get the name of this named any */
  const std::string&
  getName(){ return myName; }

  /** Set the name of this named any */
  void
  setName(const std::string& s){ myName = s; }

  // -----------------------------------------
  // Store as a wrapped smart ptr

  /** Get data we store */
  template <typename T>
  std::shared_ptr<T>
  getSP()
  {
    // the cheese of wrapping vector around shared_ptr
    try{
      auto back = boost::any_cast<std::shared_ptr<T> >(myData);
      return back;
    }catch (boost::bad_any_cast) {
      return nullptr;
    }
  }

  /** Is this data this type? */
  template <typename T>
  bool
  isSP()
  {
    return (myData.type() == typeid(std::shared_ptr<T> ));
  }

  // -----------------------------------------
  // Store as an something, but return wrapped optional

  /** Get data we store as a boost optional */
  template <typename T>
  boost::optional<T>
  get()
  {
    try{
      auto back = boost::any_cast<T>(myData);
      return boost::optional<T>(back);
    }catch (boost::bad_any_cast) { }
    return boost::none;
  }

  /** Is this data this type? */
  template <typename T>
  bool
  is()
  {
    return (myData.type() == typeid(T));
  }

  /** Set the data we store */
  void
  set(boost::any&& in)
  {
    myData = std::move(in);
  }

protected:

  /** Name of this data */
  std::string myName;

  /** We can store ANYTHING!!!!! */
  boost::any myData;
};

/** A mapping class for NamedAny objects*/
class NamedAnyList : public Data
{
public:

  /** Allow begin to iterate through our elements */
  NamedAny *
  begin()
  {
    return &myAttributes[0];
  }

  /** Allow end to iterate through our elements */
  NamedAny *
  end()
  {
    return &myAttributes[myAttributes.size()]; // note it's one past end
  }

  /** Return index into the storage for this name */
  int
  index(const std::string& name)
  {
    size_t count = 0;

    for (auto i:myAttributes) {
      if (i.getName() == name) {
        return count; // always +
      }
      count++;
    }
    return -1;
  }

  /** Add something with given key */
  template <typename T>
  void
  put(const std::string& name, const T& value)
  {
    int i = index(name);

    // Either set the old one to new data....
    if (i != -1) {
      myAttributes[i].setName(name);
      myAttributes[i].set(value);
    } else {
      // Or add a new node ...
      NamedAny g(name);
      g.set(value);

      myAttributes.push_back(g);
    }
  }

  /** Get data we store as an optional */
  template <typename T>
  boost::optional<T>
  get(const std::string& name)
  {
    int i = index(name);

    if (i != -1) {
      return myAttributes[i].get<T>();
    }
    return boost::none;
  }

protected:

  /** Stored attributes */
  std::vector<NamedAny> myAttributes;
};

/** Store list of data attributes for something */
class DataAttributeList : public NamedAnyList
{
  // Nothing extra at moment.  Could wipe this class
  // or do a typedef..will keep for moment
};

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

  /** New one */
  DataAttributeList myAttributes;

protected:

  /** String used for writer/reader factory, such as 'RadialSet' */
  std::string myDataType;

  /** The TypeName of the data contained.  */
  std::string myTypeName;

  /** Attribute storage */
  DataAttributes myAttributes2;
};
}
