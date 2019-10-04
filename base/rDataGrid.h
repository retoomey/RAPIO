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

/* Holds a reference from a name to data
 * @author Robert Toomey
 */
class DataNode : public Data
{
public:
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

  /** Get the dimension reference */
  std::vector<int>
  getDims(){ return myDims; }

  /** The name of the data */
  std::string myName;

  /** The units of the data */
  std::string myUnits;

  /** The index number of dimensions of the data */
  std::vector<int> myDims;

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
};

class DataNameCollection : public Data
{
public:
  std::vector<std::shared_ptr<DataNode> > myNodes;

  template <typename T>
  std::shared_ptr<T>
  add(const std::string& name, const std::string& units,
    T&& aT, int dim1 = 0, int dim2 = -1, int dim3 = -1)
  {
    std::shared_ptr<T> ptr = std::make_shared<T>(std::forward<T>(aT));
    auto newNode = std::make_shared<DataNode>();

    // Required attributes
    newNode->myName  = name;
    newNode->myUnits = units;

    // Store references to dimension number...
    newNode->myDims.push_back(dim1);
    if (dim2 > -1) {
      newNode->myDims.push_back(dim2);
    }
    if (dim3 > -1) {
      newNode->myDims.push_back(dim3);
    }

    // Data
    // newNode->set(std::make_shared<T>(std::forward<T>(aT)));
    newNode->set(ptr);
    myNodes.push_back(newNode);

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
    return myData.myNodes;
  }

  // 1D stuff ----------------------------------------------------------

  /** Get back object so can call methods on it */
  std::shared_ptr<RAPIO_1DF>
  getFloat1D(const std::string& name);

  /** Add named float data with initial size and value */
  std::shared_ptr<RAPIO_1DF>
  addFloat1D(const std::string& name, const std::string& units, size_t size, float value);

  /** Add named float data with initial size and value (uninitialized) */
  std::shared_ptr<RAPIO_1DF>
  addFloat1D(const std::string& name, const std::string& units, size_t size);

  /** Resize a float data */
  void
  resizeFloat1D(const std::string& name, size_t size, float value);

  /** Copy a float data into another, making it the source size */
  void
  copyFloat1D(const std::string& from, const std::string& to);

  /** Get a pointer data for quick transversing */
  std::shared_ptr<RAPIO_1DI>
  getInt1D(const std::string& name);

  /** Add named int data with initial size and value */
  std::shared_ptr<RAPIO_1DI>
  addInt1D(const std::string& name, const std::string& units, size_t size, int value);

  /** Add named int data with initial size and value (uninitialized) */
  std::shared_ptr<RAPIO_1DI>
  addInt1D(const std::string& name, const std::string& units, size_t size);

  /** Resize a float data */
  void
  resizeInt1D(const std::string& name, size_t size, int value);

  // 2D stuff ----------------------------------------------------------

  /** Get back node so can call methods on it */
  std::shared_ptr<DataNode>
  getDataNode(const std::string& name);

  /** Get back object so can call methods on it */
  std::shared_ptr<RAPIO_2DF>
  getFloat2D(const std::string& name);

  /** Add named 2D float data with initial size and value */
  std::shared_ptr<RAPIO_2DF>
  addFloat2D(const std::string& name, const std::string& units, size_t numx, size_t numy, float value);

  /** Add named float data with initial size and value */
  std::shared_ptr<RAPIO_2DF>
  addFloat2D(const std::string& name, const std::string& units, size_t numx, size_t numy);

  /** Resize a 2D float data */
  void
  resizeFloat2D(const std::string& name, size_t numx, size_t numy, float value);

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

  /** Resizes primary 2D grid to a ROW x COL grid and sets each cell to `value' */
  virtual void
  resize(size_t numx, size_t numy, const float fill = 0);
  // ----------------------------------------------------------------------

protected:

  /** Collection of named data object */
  DataNameCollection myData;
};
}
