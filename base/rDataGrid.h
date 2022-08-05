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

  // 1D stuff ----------------------------------------------------------

  /** Get back object so can call methods on it */
  inline std::shared_ptr<Array<float, 1> >
  getFloat1D(const std::string& name = Constants::PrimaryDataName)
  {
    return get<Array<float, 1> >(name);
  }

  /** Get back primary (first) grid reference */
  inline boost::multi_array<float, 1>&
  getFloat1DRef(const std::string& name = Constants::PrimaryDataName)
  {
    return get<Array<float, 1> >(name)->ref();
  }

  /** Add named float data with initial size and value (uninitialized) */
  std::shared_ptr<Array<float, 1> >
  addFloat1D(const std::string& name, const std::string& units, const std::vector<size_t>& dimindexes);

  /** Get a pointer data for quick transversing */
  inline std::shared_ptr<Array<int, 1> >
  getInt1D(const std::string& name = Constants::PrimaryDataName)
  {
    return get<Array<int, 1> >(name);
  }

  /** Get back primary (first) grid reference */
  inline boost::multi_array<int, 1>&
  getInt1DRef(const std::string& name = Constants::PrimaryDataName)
  {
    return get<Array<int, 1> >(name)->ref();
  }

  /** Add named int data with initial size and value (uninitialized) */
  std::shared_ptr<Array<int, 1> >
  addInt1D(const std::string& name, const std::string& units, const std::vector<size_t>& dimindexes);

  // 2D stuff ----------------------------------------------------------

  /** Get back object so can call methods on it */
  inline std::shared_ptr<Array<float, 2> >
  getFloat2D(const std::string& name = Constants::PrimaryDataName)
  {
    return get<Array<float, 2> >(name);
  }

  /** Get ref to a named 2d array */
  inline boost::multi_array<float, 2>&
  getFloat2DRef(const std::string& name = Constants::PrimaryDataName)
  {
    return get<Array<float, 2> >(name)->ref();
  }

  /** Add named float data with initial size and value */
  std::shared_ptr<Array<float, 2> >
  addFloat2D(const std::string& name, const std::string& units, const std::vector<size_t>& dimindexes);

  // 3D stuff ----------------------------------------------------------

  /** Get back object so can call methods on it */
  inline std::shared_ptr<Array<float, 3> >
  getFloat3D(const std::string& name = Constants::PrimaryDataName)
  {
    return get<Array<float, 3> >(name);
  }

  /** Get ref to a named 2d array */
  inline boost::multi_array<float, 3>&
  getFloat3DRef(const std::string& name = Constants::PrimaryDataName)
  {
    return get<Array<float, 3> >(name)->ref();
  }

  /** Add named float data with initial size and value */
  std::shared_ptr<Array<float, 3> >
  addFloat3D(const std::string& name, const std::string& units, const std::vector<size_t>& dimindexes);

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

    // Set with type security
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
