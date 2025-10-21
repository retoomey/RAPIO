#pragma once

#include <rUtility.h>
#include <rDataType.h>
#include <rDataStore2D.h>
#include <rArray.h>
#include <rPTreeData.h>
#include <rDataArray.h>
#include <rBOOST.h>

BOOST_WRAP_PUSH
#include <boost/multi_array.hpp>
#include <boost/variant.hpp>
#include <boost/any.hpp>
#include <boost/optional.hpp>
BOOST_WRAP_POP

#include <vector>
#include <stdexcept>

namespace rapio {
/* Exception for calling any Ref function that is assuming an array exists */
class ArrayRefException : public Utility, public std::exception
{
public:
  ArrayRefException(const std::string& typeString, int dimension, const std::string& name)
    : myMessage("Array does not exist to reference: TYPE=" + typeString
      + ", DIMENSION=" + std::to_string(dimension)
      + ", NAME=" + name){ }

  virtual const char *
  what() const noexcept override
  {
    return myMessage.c_str();
  }

protected:
  std::string myMessage;
};

/* Macro for declaring all the convenience methods to help avoid mistyping
 * This creates getByte1D, getByte1DRef and addByte1D, addByte1DRef methods, etc.
 */
#define DeclareArrayMethodsForD(TYPESTRING, TYPE, ARRAYTYPE, DIMENSION) \
  inline std::shared_ptr<Array<TYPE, DIMENSION> > \
  get ## TYPESTRING ## DIMENSION ## D(const std::string& name = Constants::PrimaryDataName) const \
  { \
    return get<Array<TYPE, DIMENSION> >(name); \
  } \
  inline boost::multi_array<TYPE, DIMENSION>& \
  get ## TYPESTRING ## DIMENSION ## DRef(const std::string& name = Constants::PrimaryDataName) const \
  { \
    auto temp = get<Array<TYPE, DIMENSION> >(name); \
    if (!temp) { \
      throw ArrayRefException(#TYPESTRING, DIMENSION, name); \
    } \
    else { \
      return temp->ref(); \
    } \
  } \
  inline boost::multi_array<TYPE, DIMENSION> * \
    get ## TYPESTRING ## DIMENSION ## DPtr(const std::string& name = Constants::PrimaryDataName) const \
  { \
    auto temp = get<Array<TYPE, DIMENSION> >(name); \
    return (temp ? temp->ptr() : nullptr); \
  } \
  std::shared_ptr<Array<TYPE, DIMENSION> > \
  add ## TYPESTRING ## DIMENSION ## D(const std::string& name, const std::string& units, \
    const std::vector<size_t>& dimindexes, TYPE fillValue = TYPE()) \
  { \
    return add<TYPE, DIMENSION>(name, units, ARRAYTYPE, dimindexes, fillValue); \
  } \
  inline boost::multi_array<TYPE, DIMENSION>& \
  add ## TYPESTRING ## DIMENSION ## DRef(const std::string& name, const std::string& units, \
    const std::vector<size_t>& dimindexes, TYPE fillValue = TYPE()) \
  { \
    return add<TYPE, DIMENSION>(name, units, ARRAYTYPE, dimindexes, fillValue)->ref(); \
  }

/** Declare up to 3D array access, haven't seen a need for anything higher 'yet'.
 * Make sure to sync these with the DeclareArrayFactoryMethods in the .cc  */
#define DeclareArrayMethods(TYPESTRING, TYPE, ARRAYTYPE) \
  DeclareArrayMethodsForD(TYPESTRING, TYPE, ARRAYTYPE, 1) \
  DeclareArrayMethodsForD(TYPESTRING, TYPE, ARRAYTYPE, 2) \
  DeclareArrayMethodsForD(TYPESTRING, TYPE, ARRAYTYPE, 3)

/** Stores a dimension of the grid space,
 * curiously like a netcdf dimension. */
class DataGridDimension : public Data {
public:
  DataGridDimension(const std::string& name, size_t aSize) : myName(name), mySize(aSize){ }

  /** Return the number of dimensions */
  size_t
  size() const { return mySize; }

  /** Get the name of this dimension */
  std::string
  name() const { return myName; }

  /** Set the size of the dimension */
  void setSize(size_t s){ mySize = s; }

  /** Set the name of the dimension */
  void setName(const std::string& n){ myName = n; }

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

  /** Public API for users to create a DataGrid,
   * usually you create a subclass like RadialSet.  This is
   * used if you need to create generic array data. */
  static std::shared_ptr<DataGrid>
  Create(const std::string         & aTypeName,
    const std::string              & Units,
    const LLH                      & location,
    const Time                     & datatime,
    const std::vector<size_t>      & dimsizes,
    const std::vector<std::string> & dimnames);

  /** Public API for users to clone a DataGrid */
  std::shared_ptr<DataGrid>
  Clone();

  /** Resize existing dimensions given a vector list */
  void
  resize(const std::vector<size_t>& dimsizes);

  /** Use a parameter pack so you can call resize(x,y,z...) */
  template <typename ... Sizes>
  void
  resize(Sizes... dimsizes)
  {
    std::vector<size_t> dimVec{ dimsizes ... };

    resize(dimVec);
  }

  /** Destroy a DataGrid */
  virtual ~DataGrid(){ }

  /** Declare the dimensions of array objects. */
  void
  setDims(const std::vector<size_t>& dimsizes,
    const std::vector<std::string> & dimnames);

  /** Add a new named dimension of given size to the datagrid.
   * This shouldn't need to change current arrays since it's adding
   */
  void
  addDim(const std::string& dimname, size_t dimsize)
  {
    myDims.push_back(DataGridDimension(dimname, dimsize));
  };

  /** Get back node so can call methods on it */
  inline std::shared_ptr<DataArray>
  getDataArray(const std::string& name = Constants::PrimaryDataName)
  {
    return getNode(name);
  }

  /** Update global attribute list for RadialSet */
  virtual void
  updateGlobalAttributes(const std::string& encoded_type) override;

  /** Return a units for this DataType for a given key.  Some DataTypes have subfields,
   * subarrays that have multiple unit types */
  virtual std::string
  getUnits(const std::string& name = Constants::PrimaryDataName) override;

  /** Set the primary units.  Some DataTypes have subfields,
   * subarrays that have multiple unit types */
  virtual void
  setUnits(const std::string& units, const std::string& name = Constants::PrimaryDataName) override;

  /** Sync any internal stuff to data from current attribute list,
   * return false on fail. */
  virtual bool
  initFromGlobalAttributes() override;

  // Non-template array access -----------------------------------------
  // NOTE: Adding DeclareArrayMethods here, you need to add to the
  // DeclareFactoryArrayMethods in the factoryGetRawDataPointer method

  /** Functions for referencing signed byte arrays of 8 bit */
  DeclareArrayMethods(Byte, int8_t, BYTE)

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

  /** Return dimensions */
  std::vector<DataGridDimension>
  getDims(){ return myDims; }

  /** Return size of each dimension */
  std::vector<size_t>
  getSizes();

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
    const std::vector<size_t>& dimindexes, T fillValue = T())
  {
    // Map indexes to actual dimension sizes (generically)
    std::vector<size_t> sizes;

    const size_t s = myDims.size();

    for (auto x: dimindexes) {
      if (x >= s) { // 0, 1, 2  (dim = 3)
        const std::string range = s > 0 ? "[0-" + std::to_string(s - 1) + "]" : "";
        LogSevere("Attempting to add array using dimension index " << x <<
          " but we have " << myDims.size() << " dimensions " << range << ".\n");
        return nullptr;
      }
      sizes.push_back(myDims[x].size()); // FIXME: Could check sizes
    }

    auto newNode = std::make_shared<DataArray>();
    auto ptr     = newNode->init<T, S>(name, units, type, sizes, dimindexes);

    // Call fill only if the provided fillValue differs from the default-initialized value
    if (fillValue != T()) {
      ptr->fill(fillValue);
    }

    // Add or replace the named node
    int at = getNodeIndex(name);

    if (at < 0) {
      myNodes.push_back(newNode);
    } else {
      myNodes[at] = newNode;
    }

    return ptr;
  } // add

  /** Do we have an array of given name? */
  bool
  haveArrayName(const std::string& name)
  {
    for (auto i:myNodes) {
      if (i->getName() == name) {
        return true;
      }
    }
    return false;
  }

  /** Change name of a stored array.  Used for sparse/non-sparse array swapping */
  bool
  changeArrayName(const std::string& name, const std::string& newname)
  {
    for (auto i:myNodes) {
      if (i->getName() == newname) {
        LogSevere(
          "Cannot change array from " << name << " to " << newname << " since " << newname << " already exists!\n")
        return false;
      }
    }
    for (auto i:myNodes) {
      if (i->getName() == name) {
        i->setName(newname);
        return true;
      }
    }
    return false;
  }

  /** Mark hidden  Used for sparse/non-sparse array swapping */
  void
  setVisible(const std::string& name, bool flag)
  {
    for (auto i:myNodes) {
      if (i->getName() == name) {
        flag ? i->removeAttribute("RAPIO_HIDDEN") : i->putAttribute<std::string>("RAPIO_HIDDEN", "yes");
      }
    }
  }

  /** Delete a stored array.  Used for sparse/non-sparse array swapping */
  bool
  deleteArrayName(const std::string& name)
  {
    for (auto i = 0; i < myNodes.size(); ++i) {
      if (myNodes[i]->getName() == name) {
        myNodes[i] = myNodes.back(); // swap/pop delete
        myNodes.pop_back();
        return true;
      }
    }
    return false;
  }

  /** Get node for this key */
  template <typename T>
  std::shared_ptr<T>
  get(const std::string& name) const
  {
    for (auto i:myNodes) {
      if (i->getName() == name) {
        // Ok we have to cast the general interface to the specific template class
        std::shared_ptr<T> derived = std::dynamic_pointer_cast<T>(i->getArray());
        return derived;
      }
    }
    return nullptr;
  }

  /** Get node for this key */
  std::shared_ptr<DataArray>
  getNode(const std::string& name);

  /** Get index of node or -1 */
  int
  getNodeIndex(const std::string& name);

  /** Return attribute pointer for readers/writers */
  std::shared_ptr<DataAttributeList>
  getAttributes(
    const std::string& name);

  /** Create metadata Ptree for sending to python */
  std::shared_ptr<PTreeData>
  createMetadata();

  /** Default header for RAPIO */
  static double SparseThreshold;

  /** Unsparse a collection of 2D array information */
  void
  unsparse2D(size_t                   num_x,
    size_t                            num_y,
    std::map<std::string, std::string>& keys,
    const std::string                 & pixelX     = "pixel_x",
    const std::string                 & pixelY     = "pixel_y",
    const std::string                 & pixelCount = "pixel_count");

  /** Unsparse a collection of 3D array information */
  void
  unsparse3D(size_t                   num_x,
    size_t                            num_y,
    size_t                            num_z,
    std::map<std::string, std::string>& keys,
    const std::string                 & pixelX     = "pixel_x",
    const std::string                 & pixelY     = "pixel_y",
    const std::string                 & pixelZ     = "pixel_z",
    const std::string                 & pixelCount = "pixel_count");

  // We 'could' implement sparse by making a new copy of the object, which may be
  // cleaner.  Right now we hide the original data and change to look sparse for
  // any writers.  Currently these are meant to be called by writers that then
  // immediately undo the sparse so that the object can continued to be used for
  // RAM operations (the algorithm may be reusing the DataType).

  /** Sparse a collection of 3D array information, backing up the original data. */
  bool
  sparse3D();

  /** Sparse a collection of 2D array information, backing up the original data. */
  bool
  sparse2D();

  /** Restore back up of original data from sparsing, make as non-sparse again */
  void
  unsparseRestore();

protected:

  /** Deep copy our fields to a new DataGrid or subclass */
  void
  deep_copy(std::shared_ptr<DataGrid> n);

  /** Extra initialization of a DataGrid */
  bool
  init(const std::string           & aTypeName,
    const std::string              & Units,
    const LLH                      & location,
    const Time                     & datatime,
    const std::vector<size_t>      & dimsizes,
    const std::vector<std::string> & dimnames);

  /** Keep the dimensions */
  std::vector<DataGridDimension> myDims;

  /** Nodes of generic array data */
  std::vector<std::shared_ptr<DataArray> > myNodes;
};
}
