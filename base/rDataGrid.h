#pragma once

#include <rDataType.h>
#include <rDataStore2D.h>
#include <rArray.h>
#include <rPTreeData.h>

#include <vector>

#include <boost/multi_array.hpp>
#include <boost/variant.hpp>
#include <boost/any.hpp>
#include <boost/optional.hpp>

/* Macro for declaring all the convenience methods to help avoid mistyping
 * This creates getByte1D, getByte1DRef and addByte1D methods, etc.
 */
#define DeclareArrayMethodsForD(TYPESTRING, TYPE, ARRAYTYPE, DIMENSION) \
  inline std::shared_ptr<Array<TYPE, DIMENSION> > \
  get ## TYPESTRING ## DIMENSION ## D(const std::string& name = Constants::PrimaryDataName) \
  { \
    return get<Array<TYPE, DIMENSION> >(name); \
  } \
  inline boost::multi_array<TYPE, DIMENSION>& \
  get ## TYPESTRING ## DIMENSION ## DRef(const std::string& name = Constants::PrimaryDataName) \
  { \
    return get<Array<TYPE, DIMENSION> >(name)->ref(); \
  } \
  std::shared_ptr<Array<TYPE, DIMENSION> > \
  add ## TYPESTRING ## DIMENSION ## D(const std::string& name, const std::string& units, \
    const std::vector<size_t>& dimindexes) \
  { \
    return add<TYPE, DIMENSION>(name, units, ARRAYTYPE, dimindexes); \
  }

/** Declare up to 3D array access, haven't seen a need for anything higher 'yet'.
 * Make sure to sync these with the DeclareArrayFactoryMethods in the .cc  */
#define DeclareArrayMethods(TYPESTRING, TYPE, ARRAYTYPE) \
  DeclareArrayMethodsForD(TYPESTRING, TYPE, ARRAYTYPE, 1) \
  DeclareArrayMethodsForD(TYPESTRING, TYPE, ARRAYTYPE, 2) \
  DeclareArrayMethodsForD(TYPESTRING, TYPE, ARRAYTYPE, 3)

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
 * Is there a better design?
 *
 * @author Robert Toomey
 */
class DataArray : public NamedAny
{
public:

  /** Construct a DataArray with needed storage parameters */
  DataArray(const std::string& name, const std::string& units,
    const DataArrayType& type, const std::vector<size_t>& dimindexes)
    : NamedAny(name), myAttributes(std::make_shared<DataAttributeList>()), myUnits(units), myStorageType(type),
    myDimIndexes(dimindexes), myRawArrayPointer(nullptr)
  {
    myAttributes->put<std::string>("Units", units);
  }

  // The boost any is nice for general attributes, but since we store only ONE multi array
  // we'll just keep a simple shared_ptr to the non-templated interface.
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

  /** Get the DataArrayType of this data array */
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

  /** Get the array as typeless for generic usage */
  std::shared_ptr<ArrayBase>
  getNewArray(){ return myArray; }

  /** Convenience print of DataArray items.
   * FIXME: Has to be done in array to get correct template type. Might
   * be able to use a visitor class to have more control over printing. */
  void
  printArray(std::ostream& out, const std::string& indent = "    ", const std::string& divider = ", ", size_t wrap = 9)
  {
    myArray->printArray(out, indent, divider, wrap);
  }

protected:

  /** The data attribute list for this data */
  std::shared_ptr<DataAttributeList> myAttributes;

  /** The units of the data */
  std::string myUnits;

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

/** Stores a dimension of the grid space,
 * curiously like a netcdf dimension. */
class DataGridDimension : public Data {
public:
  DataGridDimension(const std::string& name, size_t aSize) : myName(name), mySize(aSize){ }

  /** Return the number of dimensions */
  size_t
  size(){ return mySize; }

  /** Get the name of this dimension */
  std::string
  name(){ return myName; }

protected:

  /** The name of the dimension size */
  std::string myName;

  /** The number of dimensions */
  size_t mySize;
};

/** DataGrid acts as a name to storage database.
 * This has common access/memory storage for multiband RadialSets
 * and LatLonGrids, etc.
 * It can generically store general netcdf files
 * FIXME: Handle netcdf groups (I currently don't use any
 * grouped data)
 *
 * @author Robert Toomey */
class DataGrid : public DataType {
public:

  /** Construct uninitialized DataGrid, usually for
   * factories.  You probably want the Create method */
  DataGrid();

  /** Create a DataGrid */
  DataGrid(const std::string       & aTypeName,
    const std::string              & Units,
    const LLH                      & center,
    const Time                     & datatime,
    const std::vector<size_t>      & dimsizes,
    const std::vector<std::string> & dimnames);

  /** Public API for users to create a DataGrid,
   * usually you create a subclass like RadialSet.  This is
   * used if you need to create generic array data. */
  static std::shared_ptr<DataGrid>
  Create(const std::string         & aTypeName,
    const std::string              & Units,
    const LLH                      & center,
    const Time                     & datatime,
    const std::vector<size_t>      & dimsizes,
    const std::vector<std::string> & dimnames);

  /** Declare the dimensions of array objects */
  void
  declareDims(const std::vector<size_t>& dimsizes,
    const std::vector<std::string>     & dimnames);

  /** Get back node so can call methods on it */
  std::shared_ptr<DataArray>
  getDataArray(const std::string& name = Constants::PrimaryDataName);

  /** Convenience to get the units of a given array name */
  virtual std::string
  getUnits(const std::string& name = Constants::PrimaryDataName) override
  {
    std::string units;

    if (name == Constants::PrimaryDataName) {
      units = myUnits;
    }
    auto n = getNode(name);

    if (n != nullptr) {
      auto someunit = n->getAttribute<std::string>("Units");
      if (someunit) {
        units = *someunit;
      }
    }
    return units;
  }

  /** Convenience to set the units of a given array name */
  virtual void
  setUnits(const std::string& units, const std::string& name = Constants::PrimaryDataName) override
  {
    if (name == Constants::PrimaryDataName) {
      myUnits = units;
    }
    auto n = getNode(name);

    if (n != nullptr) {
      n->putAttribute<std::string>("Units", units);
    }
  }

  /** Update global attribute list for RadialSet */
  virtual void
  updateGlobalAttributes(const std::string& encoded_type) override;

  /** Sync any internal stuff to data from current attribute list,
   * return false on fail. */
  virtual bool
  initFromGlobalAttributes() override;

  // Non-template array access -----------------------------------------
  // NOTE: Adding DeclareArrayMethods here, you need to add to the
  // DeclareFactoryArrayMethods in the factoryGetRawDataPointer method

  /** Functions for referencing char/Byte arrays of 8 bit */
  DeclareArrayMethods(Byte, char, BYTE)

  /** Functions for referencing short arrays of 8 bit */
  DeclareArrayMethods(Short, short, SHORT)

  /** Functions for referencing int arrays of 32 bit */
  DeclareArrayMethods(Int, int, INT)

  /** Functions for referencing float arrays of 32 bit */
  DeclareArrayMethods(Float, float, FLOAT)

  // Warning: For now at least, there's no auto 'upscaling' or downscaling,
  // you need to use direct type stored.  One trick would be to use
  // say getFloat2D("test") == nullptr then try getDouble2D("test"), etc.

  /** Functions for referencing double arrays of 64 bit */
  DeclareArrayMethods(Double, double, DOUBLE)

  /** Low level generic array creator for readers, you probably don't want this */
  void * factoryGetRawDataPointer(const std::string& name, const std::string& units, const DataArrayType& type,
    const std::vector<size_t>& dimindexes);

public:

  /** Return dimensions */
  std::vector<DataGridDimension>
  getDims(){ return myDims; }

  /** Return nodes */
  std::vector<std::shared_ptr<DataArray> >
  getArrays()
  {
    return myNodes;
  }

  /** Add an array of given type/size */
  template <typename T, unsigned int S>
  std::shared_ptr<Array<T, S> >
  add(const std::string& name, const std::string& units, const DataArrayType& type,
    const std::vector<size_t>& dimindexes)
  {
    // Map indexes to actual dimension sizes (generically)
    std::vector<size_t> sizes;

    for (auto x: dimindexes) {
      sizes.push_back(myDims[x].size()); // FIXME: Could check sizes
    }

    // Create array using the sizes of each dimenion (which holds raw array of the type)
    std::shared_ptr<Array<T, S> > ptr = std::make_shared<Array<T, S> >(sizes);

    // Make a node to store/remember this arrays attributes (name, units, dimension indexes, etc.)
    auto newNode = std::make_shared<DataArray>(name, units, type, dimindexes);

    newNode->setArray(ptr, ptr);

    // Add or replace the named node
    int at = getNodeIndex(name);

    if (at < 0) {
      myNodes.push_back(newNode);
    } else {
      myNodes[at] = newNode;
    }

    return ptr;
  }

  /** Get node for this key */
  template <typename T>
  std::shared_ptr<T>
  get(const std::string& name)
  {
    size_t count = 0;

    for (auto i:myNodes) {
      if (i->getName() == name) {
        // Ok we have to cast the general interface to the specific template class
        //
        std::shared_ptr<T> derived = std::dynamic_pointer_cast<T>(i->getNewArray());
        return derived;
        // return i->getSP<T>();
      }
      count++;
    }
    return nullptr;
  }

  /** Get node for this key */
  std::shared_ptr<DataArray>
  getNode(const std::string& name)
  {
    size_t count = 0;

    for (auto i:myNodes) {
      if (i->getName() == name) {
        return i;
      }
      count++;
    }
    return nullptr;
  }

  /** Get index of node or -1 */
  int
  getNodeIndex(const std::string& name)
  {
    int count = 0;

    for (auto i:myNodes) {
      if (i->getName() == name) {
        return count;
      }
      count++;
    }
    return -1;
  }

  /** Return attribute pointer for readers/writers */
  std::shared_ptr<DataAttributeList>
  getAttributes(
    const std::string& name);

  /** Create metadata Ptree for sending to python */
  std::shared_ptr<PTreeData>
  createMetadata();

protected:

  /** Keep the dimensions */
  std::vector<DataGridDimension> myDims;

  /** Nodes of generic array data */
  std::vector<std::shared_ptr<DataArray> > myNodes;
};
}
