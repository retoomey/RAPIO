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

#else

# define RAPIO_1DF DataStore<float>
# define RAPIO_1DI DataStore<int>
# define RAPIO_2DF DataStore2D<float>

#endif // ifdef BOOST_ARRAY

/** Type marker of data to help out reader/writers */
enum DataNodeType {
  UNKNOWN,
  FLOAT,
  INT
};

/* Holds a reference from a name to data
 * FIXME: debating even more abstract, the
 * attributes themselves such as name, units, etc.
 * could be generic and abilities merged with
 * attributes of DataType.
 *
 * @author Robert Toomey
 */
class DataNode : public Data
{
public:

  /** Construct a DataNode with needed storage parameters */
  DataNode(const std::string& name, const std::string& units,
    const DataNodeType& type, const std::vector<int>& dims)
    : myName(name), myUnits(units), myStorageType(type), myDims(dims)
  { }

  /** Get the name of this data array */
  const std::string&
  getName(){ return myName; }

  /** Set the name of this data array */
  void
  setName(const std::string& s){ myName = s; }

  /** Get the units of this data array */
  const std::string&
  getUnits(){ return myUnits; }

  /** Set the units of this data array */
  void
  setUnits(const std::string& s){ myUnits = s; }

  /** Get the DataNodeType of this data array */
  const DataNodeType&
  getStorageType(){ return myStorageType; }

  /** Set the DataNodeType of this data array */
  void
  setStorageType(const DataNodeType& s){ myStorageType = s; }

  /** Get the dimension reference */
  std::vector<int>
  getDims(){ return myDims; }

  /** Set the dimension reference */
  void
  setDims(const std::vector<int>& vin){ myDims = vin; }

  /** We can store ANYTHING!!!!! */
  boost::any myData;

  /** Set the data we store */
  void
  set(boost::any&& in)
  {
    myData = std::move(in);
  }

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

protected:

  /** The name of the data */
  std::string myName;

  /** The units of the data */
  std::string myUnits;

  /** The type of the data for reader/writers */
  DataNodeType myStorageType;

  /** The index number of dimensions of the data */
  std::vector<int> myDims;
};

class DataNameCollection : public Data
{
public:

  /** Return dimensions */
  std::vector<size_t>
  getDims(){ return myDims; }

  /** Return nodes */
  std::vector<std::shared_ptr<DataNode> >
  getArrays()
  {
    return myNodes;
  }

  /** Add a node array to our storage */
  template <typename T>
  std::shared_ptr<T>
  add(const std::string& name, const std::string& units,
    T&& aT, const DataNodeType& type, const std::vector<int>& dims = { 0 })
  {
    auto newNode = std::make_shared<DataNode>(name, units, type, dims);

    // Move and add data
    std::shared_ptr<T> ptr = std::make_shared<T>(std::forward<T>(aT));
    newNode->set(ptr);

    myNodes.push_back(newNode);

    return ptr;
  }

  /** Resize the dimensions of array objects */
  void
  declareDims(const std::vector<size_t>& dims)
  {
    myDims = dims;
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
  std::shared_ptr<DataNode>
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

protected:
  /** Nodes of generic array data */
  std::vector<std::shared_ptr<DataNode> > myNodes;

  /** Keeps the size and count of our dimensions */
  std::vector<size_t> myDims;
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

  /** Get all the nodes of the arrays we store */
  std::vector<std::shared_ptr<DataNode> >
  getArrays()
  {
    // return myData.myNodes;
    return myData.getArrays();
  }

  /** Resize the dimensions of array objects */
  void
  declareDims(const std::vector<size_t>& dims);

  /** Get back node so can call methods on it */
  std::shared_ptr<DataNode>
  getDataNode(const std::string& name);

  // 1D stuff ----------------------------------------------------------

  /** Get back object so can call methods on it */
  std::shared_ptr<RAPIO_1DF>
  getFloat1D(const std::string& name);

  /** Add named float data with initial size and value (uninitialized) */
  std::shared_ptr<RAPIO_1DF>
  addFloat1D(const std::string& name, const std::string& units, const std::vector<size_t>& dims);

  /** Copy a float data into another, making it the source size */
  // void
  // copyFloat1D(const std::string& from, const std::string& to);

  /** Get a pointer data for quick transversing */
  std::shared_ptr<RAPIO_1DI>
  getInt1D(const std::string& name);

  /** Add named int data with initial size and value (uninitialized) */
  std::shared_ptr<RAPIO_1DI>
  addInt1D(const std::string& name, const std::string& units, const std::vector<size_t>& dims);

  // 2D stuff ----------------------------------------------------------

  /** Get back object so can call methods on it */
  std::shared_ptr<RAPIO_2DF>
  getFloat2D(const std::string& name);

  /** Add named float data with initial size and value */
  std::shared_ptr<RAPIO_2DF>
  addFloat2D(const std::string& name, const std::string& units, const std::vector<size_t>& dims);

  // ----------------------------------------------------------------------
  // Access dimension stats.  Currently we have a main 'primary' 2D and
  // sub fields.  Still need to generalize this more

  /** Get the number of Y for primary 2D data grid */
  static size_t
  getY(std::shared_ptr<RAPIO_2DF> f);

  /** Get the number of X for primary 2D data grid */
  static size_t
  getX(std::shared_ptr<RAPIO_2DF> f);

  /** Replace missing/range in the primary 2D data grid */
  static void
  replaceMissing(std::shared_ptr<RAPIO_2DF> f, const float missing, const float range);
  // ----------------------------------------------------------------------

protected:

  /** Resize a int data */
  // void
  // resizeInt1D(const std::string& name, size_t size);

  /** Collection of named data object */
  DataNameCollection myData;
};
}
