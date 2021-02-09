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

namespace rapio {
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
  {
    myAttributes = std::make_shared<DataAttributeList>();
    myAttributes->put<std::string>("Units", units);
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

  /** Get a raw void pointer to array data. Used by reader/writers.
   * You probably don't want this, see the example algorithm.
   * FIXME: Maybe better as an optional */
  void *
  getRawDataPointer();

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

protected:

  /** The data attribute list for this data */
  std::shared_ptr<DataAttributeList> myAttributes;

  /** The units of the data */
  std::string myUnits;

  /** The type of the data for reader/writers */
  DataArrayType myStorageType;

  /** The index number of dimensions of the data */
  std::vector<size_t> myDimIndexes;
};

/** Stores a dimension of the grid space,
 * curiously like a netcdf dimension.
 * Use this or just two vectors.  Still debating...*/
class DataGridDimension : public Data {
public:
  DataGridDimension(const std::string& name, size_t aSize) : myName(name), mySize(aSize){ }

  size_t
  size(){ return mySize; }

  std::string
  name(){ return myName; }

protected:
  std::string myName;
  size_t mySize;
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
  declareDims(const std::vector<size_t>& dimsizes,
    const std::vector<std::string>     & dimnames);

  /** Get back node so can call methods on it */
  std::shared_ptr<DataArray>
  getDataArray(const std::string& name);

  // 1D stuff ----------------------------------------------------------

  /** Get back object so can call methods on it */
  inline std::shared_ptr<Array<float, 1> >
  getFloat1D(const std::string& name)
  {
    return get<Array<float, 1> >(name);
  }

  /** Add named float data with initial size and value (uninitialized) */
  std::shared_ptr<Array<float, 1> >
  addFloat1D(const std::string& name, const std::string& units, const std::vector<size_t>& dimindexes);

  /** Get a pointer data for quick transversing */
  inline std::shared_ptr<Array<int, 1> >
  getInt1D(const std::string& name)
  {
    return get<Array<int, 1> >(name);
  }

  /** Add named int data with initial size and value (uninitialized) */
  std::shared_ptr<Array<int, 1> >
  addInt1D(const std::string& name, const std::string& units, const std::vector<size_t>& dimindexes);

  // 2D stuff ----------------------------------------------------------

  /** Get back object so can call methods on it */
  inline std::shared_ptr<Array<float, 2> >
  getFloat2D(const std::string& name)
  {
    return get<Array<float, 2> >(name);
  }

  /** Get back primary (first) grid */
  inline std::shared_ptr<Array<float, 2> >
  getFloat2D()
  {
    return myNodes[0]->getSP<Array<float, 2> >();
  }

  /** Add named float data with initial size and value */
  std::shared_ptr<Array<float, 2> >
  addFloat2D(const std::string& name, const std::string& units, const std::vector<size_t>& dimindexes);

  /** Replace missing/range in the primary 2D data grid */
  static void
  replaceMissing(std::shared_ptr<Array<float, 2> > f, const float missing, const float range);
  // ----------------------------------------------------------------------
  //
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
        return i->getSP<T>();
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
