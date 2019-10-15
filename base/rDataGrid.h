#pragma once

#include <rDataType.h>
#include <rDataStore2D.h>

#include <vector>

#include <boost/multi_array.hpp>
#include <boost/variant.hpp>
#include <boost/any.hpp>

namespace rapio {
// Bunch of macros to allow plug and play here
// This uses boost over my DataStore.  It's a bit
// slower but the API is cleaner and expands to dimensions better
// I'm keeping DataStore for now
#define BOOST_ARRAY

#ifdef BOOST_ARRAY

# define RAPIO_1DF boost::multi_array<float, 1>
# define RAPIO_1DI boost::multi_array<int, 1>

# define RAPIO_2DF boost::multi_array<float, 2>

# define RAPIO_DIM1(x)       boost::extents[x]
# define RAPIO_DIM2(x, y)    boost::extents[x][y]
# define RAPIO_DIM3(x, y, z) boost::extents[x][y][z]

#else // ifdef BOOST_ARRAY

# define RAPIO_1DF DataStore<float>
# define RAPIO_1DI DataStore<int>
# define RAPIO_2DF DataStore2D<float>

# define RAPIO_DIM1(x)       x
# define RAPIO_DIM2(x, y)    x, y
# define RAPIO_DIM3(x, y, z) x, y, z

#endif // ifdef BOOST_ARRAY

/** Store a name to std::shared_ptr of anything pair */
class NamedAny : public Data
{
public:
  /** Create a named object */
  NamedAny(const std::string& name) : myName(name)
  { }

  /** Get the name of this data array */
  const std::string&
  getName(){ return myName; }

  /** Set the name of this data array */
  void
  setName(const std::string& s){ myName = s; }

  /** Get data we store */
  template <typename T>
  std::shared_ptr<T>
  get()
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
  is()
  {
    auto stuff = myData;

    return (myData.type() == typeid(std::shared_ptr<T> ));
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

/** Type marker of data to help out reader/writers */
enum DataArrayType {
  UNKNOWN,
  FLOAT,
  INT
};

/* Wraps a named array of data with metadata
 *
 * @author Robert Toomey
 */
class DataArray : public NamedAny
{
public:

  /** Construct a DataArray with needed storage parameters */
  DataArray(const std::string& name, const std::string& units,
    const DataArrayType& type, const std::vector<size_t>& dimindexes)
    : NamedAny(name), myUnits(units), myStorageType(type), myDimIndexes(dimindexes)
  { }

  /** Get the units of this data array */
  const std::string&
  getUnits(){ return myUnits; }

  /** Set the units of this data array */
  void
  setUnits(const std::string& s){ myUnits = s; }

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

  /** Get a raw void pointer to array data. Used by reader/writers.
   * You probably don't want this, see the example algorithm. */
  void *
  getRawDataPointer();

protected:

  /** The units of the data */
  std::string myUnits;

  /** The type of the data for reader/writers */
  DataArrayType myStorageType;

  /** The index number of dimensions of the data */
  std::vector<size_t> myDimIndexes;
};

/** DataGrid acts as a name to storage database.
 * This has common access/memory storage for multiband RadialSets
 * and LatLonGrids, etc.
 *
 * ALPHA: first pass adding multiband ability
 *
 * @author Robert Toomey */
class DataGrid : public DataType {
public:

  /** Resize the dimensions of array objects */
  void
  declareDims(const std::vector<size_t>& dims);

  /** Get back node so can call methods on it */
  std::shared_ptr<DataArray>
  getDataArray(const std::string& name);

  // 1D stuff ----------------------------------------------------------

  /** Get back object so can call methods on it */
  std::shared_ptr<RAPIO_1DF>
  getFloat1D(const std::string& name);

  /** Add named float data with initial size and value (uninitialized) */
  std::shared_ptr<RAPIO_1DF>
  addFloat1D(const std::string& name, const std::string& units, const std::vector<size_t>& dimindexes);

  /** Get a pointer data for quick transversing */
  std::shared_ptr<RAPIO_1DI>
  getInt1D(const std::string& name);

  /** Add named int data with initial size and value (uninitialized) */
  std::shared_ptr<RAPIO_1DI>
  addInt1D(const std::string& name, const std::string& units, const std::vector<size_t>& dimindexes);

  // 2D stuff ----------------------------------------------------------

  /** Get back object so can call methods on it */
  std::shared_ptr<RAPIO_2DF>
  getFloat2D(const std::string& name);

  /** Add named float data with initial size and value */
  std::shared_ptr<RAPIO_2DF>
  addFloat2D(const std::string& name, const std::string& units, const std::vector<size_t>& dimindexes);

  /** Replace missing/range in the primary 2D data grid */
  static void
  replaceMissing(std::shared_ptr<RAPIO_2DF> f, const float missing, const float range);
  // ----------------------------------------------------------------------
  //
public:

  /** Return dimensions */
  std::vector<size_t>
  getDims(){ return myDims; }

  /** Return nodes */
  std::vector<std::shared_ptr<DataArray> >
  getArrays()
  {
    return myNodes;
  }

  /** Add or replace a named node array in our storage */
  template <typename T>
  std::shared_ptr<T>
  add(const std::string& name, const std::string& units,
    T&& aT, const DataArrayType& type, const std::vector<size_t>& dims = { 0 })
  {
    // Move new data
    std::shared_ptr<T> ptr = std::make_shared<T>(std::forward<T>(aT));
    auto newNode = std::make_shared<DataArray>(name, units, type, dims);
    newNode->set(ptr);

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
        return i->get<T>();
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

protected:

  /** Nodes of generic array data */
  std::vector<std::shared_ptr<DataArray> > myNodes;

  /** Keeps the size and count of our dimensions */
  std::vector<size_t> myDims;
};
}
