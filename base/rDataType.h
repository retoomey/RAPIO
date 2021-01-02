#pragma once

#include <rData.h>
#include <rTime.h>
#include <rLLH.h>
#include <rConstants.h>
#include <rError.h>
#include <rNamedAny.h>

#include <map>
#include <string>
#include <memory>

namespace rapio {
class Time;
class TimeDuration;
class LLH;

typedef NamedAnyList DataAttributeList;

/** The abstract base class for all of the data objects.
 * Every DataType stores a location in space and time.
 * Every DataType stores a generic collection of attributes.
 */
class DataType : public Data {
public:

  /** Create empty DataType */
  DataType();

  /** Destroy a DataType */
  virtual ~DataType(){ }

  /** Format a subtype string utility function */
  static std::string
  formatString(float spec,
    size_t           tot,
    size_t           prec);

  /** Get the factory used to read this in originally, if ever.
   * This can assist writers in outputting */
  std::string
  getReadFactory() const
  {
    return (myReadFactory);
  }

  /** Set the factory used if any to create this DataType */
  void
  setReadFactory(const std::string& d)
  {
    myReadFactory = d;
  }

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

  /** Return the TypeName of this DataType. */
  const std::string &
  getTypeName() const
  {
    return myTypeName;
  }

  /** Set the TypeName of this DataType. */
  void
  setTypeName(const std::string& t)
  {
    myTypeName = t;
  }

  /** Get subtype for writing */
  bool
  getSubType(std::string& result) const;

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
  LLH
  getLocation() const
  {
    return myLocation;
  }

  /** Return the location considered the 'center' location of the datatype */
  virtual LLH
  getCenterLocation() const
  {
    return myLocation;
  }

  /** Set primary Location.  Meaning depends on subclass */
  void
  setLocation(const LLH& l)
  {
    myLocation = l;
  }

  /** Return Time that corresponds to this DataType */
  Time
  getTime(){ return myTime; }

  /** Set the Time that corresponds to this DataType */
  void
  setTime(const Time& t){ myTime = t; }

  /** Global attributes, used by reader/writers */
  std::shared_ptr<DataAttributeList>
  getGlobalAttributes(){ return myAttributes; }

  /** Get a value at a lat lon for a given layer name (SpaceTime datatype) */
  virtual double
  getValueAtLL(double latDegs, double lonDegs, const std::string& layer = "primary")
  {
    return Constants::MissingData;
  }

  /** Calculate Lat Lon coverage marching grid from spatial center */
  virtual bool
  LLCoverageCenterDegree(const float degreeOut, const size_t numRows, const size_t numCols,
    float& topDegs, float& leftDegs, float& deltaLatDegs, float& deltaLonDegs){ return false; }


protected:

  /** Global attributes for data type */
  std::shared_ptr<DataAttributeList> myAttributes;

  /** Time stamp of this datatype, used for writing */
  Time myTime;

  /** Primary location of data, say NW corner for LLG or
   * radar center for RadialSets */
  LLH myLocation;

  /** Factory reader if any, that was used to read in this data */
  std::string myReadFactory;

  /** String used for sub writer/reader factory, such as 'RadialSet' */
  std::string myDataType;

  /** The TypeName of the data contained.  */
  std::string myTypeName;
};
}
