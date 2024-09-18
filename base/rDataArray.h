#pragma once

#include <rData.h>
#include <rArray.h>
#include <rNamedAny.h>

#include <vector>

#include <boost/optional.hpp>

namespace rapio {
/** Type marker of data to help out reader/writers.  This is modeled mostly on netcdf since we use it
 * the most of all file formats. */
enum DataArrayType {
  BYTE, ///< 1 byte int.  Use for char, byte, unsigned byte
  // UBYTE, -- issue here is netcdf3 doesn't handle, so we'll keep the netcdf3 basic types for backward compatibility
  // CHAR, -- Allowed, but can just use byte
  SHORT, ///< 2 byte integer
  INT,   ///< 4 bytes on 32 and 64 bit systems
  FLOAT, ///< 4 byte floating point
  DOUBLE ///< 8 byte floating point if needed.  All our default classes use float for now
  // STRING FIXME: to do, could be useful.  Though will take special logic I think
};

/* Wraps a named array of data with metadata
 * So we have a collection of attributes as well
 * as one of the Array<T> classes
 *
 * This big thing here is that I'm trying to hide the
 * array template type stuff, since DataGrid can hold
 * a generic list of different templated arrays.
 * Treating it generic helps with reader/writer code.
 *
 * @author Robert Toomey
 */
class DataArray : public Data
{
public:

  /** Construct a blank DataArray object.  Call init */
  DataArray(){ }

  /** Initialize a DataArray and underlying Array by type.  We post init in order
   * to remove template type for general storage */
  template <typename T, unsigned int S>
  std::shared_ptr<Array<T, S> >
  init(const std::string& name, const std::string& units, const DataArrayType& type,
    const std::vector<size_t>& sizes, const std::vector<size_t>& dimindexes)
  {
    // Create attribute list for the DataArray
    myAttributes = std::make_shared<DataAttributeList>();

    myName = name;
    setString(Constants::Units, units); // Make sure the units field set to given
    myStorageType = type;
    myDimIndexes  = dimindexes;

    // Create array using the sizes of each dimension (which holds raw array of the type)
    std::shared_ptr<Array<T, S> > ptr = std::make_shared<Array<T, S> >(sizes);

    setArray(ptr, ptr);

    // LogInfo("Creating " << name << " " << units << "\n");
    return ptr;
  }

  /** Clone this array and contents. We use explicit cloning to try to avoid
   * accidental copies which hurt our real time system. */
  std::shared_ptr<DataArray>
  Clone();

  /** Get name of the array */
  std::string getName(){ return myName; }

  /** Set name of the array */
  void setName(const std::string& name){ myName = name; }

  /** Get the stored value array as typeless for generic usage */
  std::shared_ptr<ArrayBase>
  getArray(){ return myArray; }

  /** Get the array specialized as given */
  template <typename T, unsigned int S>
  std::shared_ptr<Array<T, S> >
  getArrayDerived()
  {
    auto r = std::dynamic_pointer_cast<Array<T, S> >(myArray);

    return r;
  }

  /** Assign typed Array class and keep a pointer to its data location for reader/writers */
  template <typename T>
  void
  setArray(std::shared_ptr<ArrayBase> keep, std::shared_ptr<T> derived)
  {
    myArray = keep;
    myRawArrayPointer = derived->getRawDataPointer();
  }

  /** Get a raw void pointer to array data. Used by reader/writers.
   * You probably don't want this, see the example algorithm.*/
  void *
  getRawDataPointer()
  {
    return myRawArrayPointer;
  }

  /** Get the DataArrayType of this data array.  Used by reader/writers
   * where separate C functions are called per type. */
  const DataArrayType&
  getStorageType(){ return myStorageType; }

  /** Set the DataArrayType of this data array */
  void
  setStorageType(const DataArrayType& s){ myStorageType = s; }

  /** Get the dimension reference */
  std::vector<size_t>
  getDimIndexes(){ return myDimIndexes; }

  /** Set the dimension reference */
  void
  setDimIndexes(const std::vector<size_t>& vin){ myDimIndexes = vin; }

  /** Get attributes list.  Used by reader/writers. */
  std::shared_ptr<DataAttributeList>
  getAttributes();

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

  // ----------------------------------------------
  // Lower level access.  Typically not needed
  // I might deprecate these actually.

  /** Get attribute */
  template <typename T>
  boost::optional<T>
  getAttribute(const std::string& name)
  {
    return myAttributes->get<T>(name);
  }

  /** Put attribute */
  template <typename T>
  void
  putAttribute(const std::string& name, const T& value)
  {
    myAttributes->put<T>(name, value);
  }

  /** Remove attribute */
  void
  removeAttribute(const std::string& name)
  {
    myAttributes->remove(name);
  }

  /** Count attributes */
  size_t
  getAttributesCount()
  {
    return myAttributes->size();
  }

  /** Convenience print of DataArray items.
   * FIXME: Has to be done in array to get correct template type. Might
   * be able to use a visitor class to have more control over printing. */
  void
  printArray(std::ostream& out = std::cout, const std::string& indent = "    ", const std::string& divider = ", ",
    size_t wrap = 9)
  {
    myArray->printArray(out, indent, divider, wrap);
  }

protected:

  /** The name of this array */
  std::string myName;

  /** The data attribute list for this data */
  std::shared_ptr<DataAttributeList> myAttributes;

  /** The type of the data for reader/writers */
  DataArrayType myStorageType;

  /** The index number of dimensions of the data */
  std::vector<size_t> myDimIndexes;

  // General internal access methods where we remove type

  /** The Array we store (type hidden here) */
  std::shared_ptr<ArrayBase> myArray;

  /** A raw pointer to our last set array. Valid only
   * when we are. Safest would be a weak-ref might do that
   * later on.  You should be using getNArray functions with auto */
  void * myRawArrayPointer;
};
}
