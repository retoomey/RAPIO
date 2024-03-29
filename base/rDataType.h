#pragma once

#include <rAttributeDataType.h>
#include <rTime.h>
#include <rLLH.h>
#include <rConstants.h>
#include <rError.h>
#include <rNamedAny.h>
#include <rDataProjection.h>

#include <string>
#include <memory>

namespace rapio {
class TimeDuration;
class ColorMap;

/** The abstract base class for all of the GIS MRMS/HMET data objects.
 * Every DataType stores a location in space and time.
 */
class DataType : public AttributeDataType {
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

  /** An assigned ID for this DataType.  It's up to the caller to
  * decide ID rotation/reuse.  For example, in the fusion algorithm we need
  * to store large arrays referring to recent DataTypes.  By rotating an 8 byte id,
  * or possibly a 16 byte id we can save memory vs by pointer. */
  int
  getID() const
  {
    return myID;
  }

  /** Assign an ID number.  Used by algorithms for whatever they want */
  inline void
  setID(int id)
  {
    myID = id;
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
  getCenterLocation()
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

  /** Sync any internal stuff to data from current attribute list,
   * return false on fail. */
  virtual bool
  initFromGlobalAttributes();

  /** Make sure global attribute list reflects any internal variables */
  virtual void
  updateGlobalAttributes(const std::string& encoded_type);

  /** Projection for data type */
  virtual std::shared_ptr<DataProjection>
  getProjection(const std::string& layer = "primary"){ return nullptr; }

  /** Return a units for this DataType for a given key.  Some DataTypes have subfields,
   * subarrays that have multiple unit types */
  virtual std::string getUnits(const std::string& name = Constants::PrimaryDataName)
  { return myUnits; }

  /** Set the primary units.  Some DataTypes have subfields,
   * subarrays that have multiple unit types */
  virtual void setUnits(const std::string& units, const std::string& name = Constants::PrimaryDataName)
  { myUnits = units; }

  /** Prepare DataType view for writing.  This can mean creating sparse arrays, etc. */
  virtual void preWrite(bool sparse = false){ }

  /** Post DataType after writing.  This can destroy temp arrays, etc. that were made just for writing. */
  virtual void postWrite(bool sparse = false){ }

  /** Get the ColorMap for converting values to colors */
  virtual std::shared_ptr<ColorMap>
  getColorMap();

  /** The default prefix pattern (or local path) for writing datatype files,
   * this is a tree using datatable, subtype and timestamp name.
   *
   * {base} Calculated base prefix, usually based on parameters.
   * {datatype} Datatype string.
   * {/subtype} If subtype not empty adds a '/' and subtype.
   * {_subtype} If subtype not empty, add a '_' and subtype.
   * {subtype}  Adds a subtype always.
   * {time}     Shortcut for a stock time string pattern.
   */
  static std::string DATATYPE_PREFIX;

  /** The default prefix pattern for the subdirs flat path from WDSS2, this
   * puts more files into a single directory */
  static std::string DATATYPE_PREFIX_FLAT;

  /** Move this datatype contexts to another.
   * Shared pointers are just referenced, raw types are copied.
   * Used for example by specializers to specialize DataTypes into subclasses where
   * typically the original DataType class is let expire. */
  void
  Move(std::shared_ptr<DataType> to)
  {
    to->myAttributes  = myAttributes;
    to->myTime        = myTime;
    to->myLocation    = myLocation;
    to->myReadFactory = myReadFactory;
    to->myDataType    = myDataType;
    to->myID       = myID;
    to->myTypeName = myTypeName;
    to->myUnits    = myUnits;
  }

protected:

  /** Time stamp of this datatype, used for writing */
  Time myTime;

  /** Primary location of data, say NW corner for LLG or
   * radar center for RadialSets */
  LLH myLocation;

  /** Factory reader if any, that was used to read in this data */
  std::string myReadFactory;

  /** String used for sub writer/reader factory, such as 'RadialSet' */
  std::string myDataType;

  /** The ID of the data contained.  */
  int myID;

  /** The TypeName of the data contained.  */
  std::string myTypeName;

  /** The primary units of the data contained.  */
  std::string myUnits;

  /** Cached projection object for GIS */
  std::shared_ptr<DataProjection> myDataProjection;
};
}
