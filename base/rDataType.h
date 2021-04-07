#pragma once

#include <rData.h>
#include <rTime.h>
#include <rLLH.h>
#include <rConstants.h>
#include <rError.h>
#include <rNamedAny.h>
#include <rDataProjection.h>

#include <map>
#include <string>
#include <memory>

namespace rapio {
class TimeDuration;
class ColorMap;

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
  std::string
  getSubType() const;

  /** Set subtype */
  void
  setSubType(const std::string& s);

  /** Generated subtype for outputting from empty */
  virtual std::string
  getGeneratedSubtype() const
  {
    return ("");
  }

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

  // ------------------------------------------------------------------
  // Global attribute API.  Netcdf uses this for its general list
  // of global attributes so something set here goes directly
  // to global netcdf attribute list
  //
  // FIXME: Maybe subclass this or pure interface to reduce code
  // clutter.  Doing it non-template to make it easier for students,
  // etc. that are more confused by template arguments.

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

  /** Global attributes, used by reader/writers */
  std::shared_ptr<DataAttributeList>
  getGlobalAttributes(){ return myAttributes; }

  /** Projection for data type */
  virtual std::shared_ptr<DataProjection>
  getProjection(const std::string& layer = "primary"){ return nullptr; }

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

  // End global attribute API
  // ------------------------------------------------------------------

  /** Get the ColorMap for converting values to colors */
  virtual std::shared_ptr<ColorMap>
  getColorMap();

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
