#pragma once

#include <rDataType.h>
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
#include <array>
#include <stdexcept>
#include <string_view>

namespace rapio {
/* Exception for calling any Ref function that is assuming an array exists */
class ArrayRefException : public std::exception
{
public:
  ArrayRefException(std::string_view typeString, int dimension, std::string_view name)
    : myMessage("Array does not exist to reference: TYPE=" + std::string(typeString)
      + ", DIMENSION=" + std::to_string(dimension)
      + ", NAME=" + std::string(name)){ }

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
  get ## TYPESTRING ## DIMENSION ## D(std::string_view name = Constants::PrimaryDataName) const \
  { \
    return get<Array<TYPE, DIMENSION> >(name); \
  } \
  inline boost::multi_array<TYPE, DIMENSION>& \
  get ## TYPESTRING ## DIMENSION ## DRef(std::string_view name = Constants::PrimaryDataName) const \
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
    get ## TYPESTRING ## DIMENSION ## DPtr(std::string_view name = Constants::PrimaryDataName) const \
  { \
    auto temp = get<Array<TYPE, DIMENSION> >(name); \
    return (temp ? temp->ptr() : nullptr); \
  } \
  std::shared_ptr<Array<TYPE, DIMENSION> > \
  add ## TYPESTRING ## DIMENSION ## D(std::string_view name, std::string_view units, \
    const std::vector<size_t>& dimindexes, TYPE fillValue = TYPE()) \
  { \
    return add<TYPE, DIMENSION>(name, units, ARRAYTYPE, dimindexes, fillValue); \
  } \
  inline boost::multi_array<TYPE, DIMENSION>& \
  add ## TYPESTRING ## DIMENSION ## DRef(std::string_view name, std::string_view units, \
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
class DataGridDimension {
public:
  DataGridDimension(std::string_view name, size_t aSize) : myName(name), mySize(aSize){ }

  /** Return the number of dimensions */
  size_t
  size() const { return mySize; }

  /** Get the name of this dimension */
  const std::string&
  name() const { return myName; }

  /** Set the size of the dimension */
  void setSize(size_t s){ mySize = s; }

  /** Set the name of the dimension */
  void setName(std::string_view n){ myName = n; }

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
  friend class SparseEncoder;
public:

  /** Construct uninitialized DataGrid, usually for
   * factories.  You probably want the Create method */
  DataGrid();

  /** Public API for users to create a DataGrid,
   * usually you create a subclass like RadialSet.  This is
   * used if you need to create generic array data. */
  static std::shared_ptr<DataGrid>
  Create(std::string_view          aTypeName,
    std::string_view               Units,
    const LLH                      & location,
    const Time                     & datatime,
    const std::vector<size_t>      & dimsizes,
    const std::vector<std::string> & dimnames);

  /** Public API for users to clone a DataGrid */
  std::shared_ptr<DataGrid>
  Clone() const;

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
  addDim(std::string_view dimname, size_t dimsize)
  {
    myDims.emplace_back(dimname, dimsize);
  };

  void
  popDim()
  {
    if (!myDims.empty()) {
      myDims.pop_back();
    }
  }

  /** Get back node so can call methods on it */
  inline std::shared_ptr<DataArray>
  getDataArray(std::string_view name = Constants::PrimaryDataName)
  {
    return getNode(name);
  }

  /** Update global attribute list for RadialSet */
  virtual void
  updateGlobalAttributes(std::string_view encoded_type) override;

  /** Return a units for this DataType for a given key.  Some DataTypes have subfields,
   * subarrays that have multiple unit types */
  virtual std::string
  getUnits(std::string_view name = Constants::PrimaryDataName) override;

  /** Set the primary units.  Some DataTypes have subfields,
   * subarrays that have multiple unit types */
  virtual void
  setUnits(std::string_view units, std::string_view name = Constants::PrimaryDataName) override;

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
  void * factoryGetRawDataPointer(std::string_view name, std::string_view units, const DataArrayType& type,
    const std::vector<size_t>& dimindexes);

  /** Return dimensions */
  const std::vector<DataGridDimension>&
  getDims() const { return myDims; }

  /** Return size of each dimension */
  const std::vector<size_t>
  getSizes() const;

  /** Return nodes */
  std::vector<std::shared_ptr<DataArray> >
  getArrays()
  {
    return myNodes;
  }

  /** Return only the visible nodes (skips arrays flagged RAPIO_HIDDEN).
   * Used by writers so sparsed/hidden backing arrays aren't written. */
  std::vector<std::shared_ptr<DataArray> >
  getVisibleArrays()
  {
    std::vector<std::shared_ptr<DataArray> > visible;

    for (const auto& l:myNodes) {
      auto hidden = l->getAttribute<std::string>("RAPIO_HIDDEN");
      if (!hidden) {
        visible.push_back(l);
      }
    }
    return visible;
  }

  /** Add an array of given type/size */
  template <typename T, unsigned int S>
  std::shared_ptr<Array<T, S> >
  add(std::string_view name, std::string_view units, const DataArrayType& type,
    const std::vector<size_t>& dimindexes, T fillValue = T())
  {
    // Map indexes to actual dimension sizes (generically)
    std::vector<size_t> sizes;

    const size_t s = myDims.size();

    for (auto x: dimindexes) {
      if (x >= s) { // 0, 1, 2  (dim = 3)
        const std::string range = s > 0 ? "[0-" + std::to_string(s - 1) + "]" : "";
        fLogSevere("Attempting to add array using dimension index {} but we have {} dimensions {}.",
          x, myDims.size(), range);
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
  haveArrayName(std::string_view name) const
  {
    for (const auto& i:myNodes) {
      if (i->getName() == name) {
        return true;
      }
    }
    return false;
  }

  /** Change name of a stored array.  Used for sparse/non-sparse array swapping */
  bool
  changeArrayName(std::string_view name, std::string_view newname)
  {
    for (const auto& i:myNodes) {
      if (i->getName() == newname) {
        fLogSevere("Cannot change array from {} to {} since {} already exists!",
          name, newname, newname);
        return false;
      }
    }
    for (const auto& i:myNodes) {
      if (i->getName() == name) {
        i->setName(newname);
        return true;
      }
    }
    return false;
  }

  /** Mark hidden  Used for sparse/non-sparse array swapping */
  void
  setVisible(std::string_view name, bool flag)
  {
    for (const auto& i:myNodes) {
      if (i->getName() == name) {
        flag ? i->removeAttribute("RAPIO_HIDDEN") : i->putAttribute<std::string>("RAPIO_HIDDEN", "yes");
      }
    }
  }

  /** Delete a stored array.  Used for sparse/non-sparse array swapping */
  bool
  deleteArrayName(std::string_view name)
  {
    for (size_t i = 0; i < myNodes.size(); ++i) {
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
  get(std::string_view name) const
  {
    for (const auto& i:myNodes) {
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
  getNode(std::string_view name);

  /** Get index of node or -1 */
  int
  getNodeIndex(std::string_view name);

  /** Return attribute pointer for readers/writers */
  std::shared_ptr<DataAttributeList>
  getAttributes(
    std::string_view name);

  /** Create metadata Ptree for sending to python */
  std::shared_ptr<PTreeData>
  createMetadata();

protected:

  /** Deep copy our fields to a new DataGrid or subclass */
  void
  deep_copy(const std::shared_ptr<DataGrid>& n) const;

  /** Extra initialization of a DataGrid */
  bool
  init(std::string_view            aTypeName,
    std::string_view               Units,
    const LLH                      & location,
    const Time                     & datatime,
    const std::vector<size_t>      & dimsizes,
    const std::vector<std::string> & dimnames);

  /** Keep the dimensions */
  std::vector<DataGridDimension> myDims;

  /** Nodes of generic array data */
  std::vector<std::shared_ptr<DataArray> > myNodes;

private:
  #if 0
  template <size_t N>
  bool
  sparseT(IOConfig& keys);

  template <size_t N>
  void
  unsparseT(IOConfig& keys, const std::array<std::string, N>& axisNames, const std::string& countName);
  #endif
};
}
