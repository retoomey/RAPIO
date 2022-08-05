#pragma once

#include "rData.h"
#include "rNamedAny.h"
#include "rConstants.h"

#include <memory>
#include <iostream>

#include <boost/multi_array.hpp>

namespace rapio {
/* Base no-template typeless interface class
 * This allows some generic typeless access and convenient
 * generic vector pointer storage say in DataGrid
 */
class ArrayBase : public Data
{
public:

  /** Create array using an initializer list for dimension sizes */
  ArrayBase(std::initializer_list<size_t> dims)
  {
    myDimSize = dims.size(); // default size
    for (auto x:dims) {
      myDims.push_back(x);
    }
    // Subclasses should call in constructor to call theirs
    // syncToDims();
  }

  /** Create array from vector of dimension sizes */
  ArrayBase(const std::vector<size_t>& dims) : myDims(dims)
  {
    myDimSize = dims.size(); // default size
    // Subclasses should call in constructor to call theirs
    // syncToDims();
  }

  /** Stolen from the template */
  size_t
  getNumDimensions()
  {
    return myDimSize;
  }

  /** Resize using an initialize list of dimensions */
  void
  resize(std::initializer_list<size_t> dims)
  {
    myDims.clear();
    for (auto x:dims) {
      myDims.push_back(x);
    }
    syncToDims(); // virtual
  }

  /** Resize using a vector list of dimensions */
  void
  resize(const std::vector<size_t>& dims)
  {
    myDims = dims;
    syncToDims(); // virtual
  }

  /** Get a vector of the sizes for each dimension */
  const std::vector<size_t>&
  getSizes() const
  {
    return myDims;
  }

  /** Create the array for current dimensions */
  virtual bool
  syncToDims() = 0;

  /** Return a raw data pointer if possible */
  virtual void *
  getRawDataPointer() = 0;

protected:
  /** Vector of sizes for each dimension */
  std::vector<size_t> myDims;

  /** Save a count of our true number of dimensions, which can differ from passed in dimension
   * settings. */
  size_t myDimSize;
};

/* Storage of an array API.  Wraps another storage system.
 * We do this to hide the details of whatever storage
 * we're using so we can swap libraries if needed.
 *
 * Create directly:
 * Array<float, 2>({100,100});
 * Data is uninitialized for speed 'usually' all cells are
 * touched by algorithms.  You can call fill to fill with
 * a particular value.
 *
 * @author Robert Toomey
 */
template <typename C, size_t N> class Array : public ArrayBase
{
public:

  /** Create array using an initializer list for dimension sizes */
  Array(std::initializer_list<size_t> dims) : ArrayBase(dims)
  {
    myDimSize = N; // Our Permanent Dimension size
    syncToDims();  // Call ours to update storage
  }

  /** Create array from vector of dimension sizes */
  Array(const std::vector<size_t>& dims) : ArrayBase(dims)
  {
    myDimSize = N; // Our Permanent Dimension size
    syncToDims();  // Call ours to update storage
  }

  /** Fill array with given value, requires storage knowledge  */
  void
  fill(C value)
  {
    auto& dd = ref();

    boost::multi_array_ref<float, 1> a_ref(dd.data(), boost::extents[dd.num_elements()]);

    std::fill(a_ref.begin(), a_ref.end(), value);
  }

  /** Return vector of current dimensions of data */
  std::vector<size_t>
  dims()
  {
    std::vector<size_t> theDims;
    auto& dd = ref();
    auto s   = dd.shape();

    for (size_t i = 0; i < N; i++) {
      theDims.push_back(s[i]);
    }
    return theDims;
  }

  /** Create the array for current dimensions */
  virtual bool
  syncToDims() override
  {
    // FIXME: We need a deep array test.  Possible that changing dimensions
    // will break.  Most use cases we don't change dimensions
    myStorage.resize(myDims);
    return true;
  }

  /** Dump entire array to std::cout.  Used for debugging */
  void
  dump()
  {
    auto& dd = ref();

    boost::multi_array_ref<float, 1> a_ref(dd.data(), boost::extents[dd.num_elements()]);

    LogInfo("Dumping array for  " << (void *) (&(dd)) << "\n");
    for (auto i = a_ref.begin(); i != a_ref.end(); ++i) {
      std::cout << *i << ", ";
    }
    std::cout << "\n";
  };

  /** Get a reference to raw array for iteration */
  boost::multi_array<C, N>&
  ref()
  {
    return myStorage;
  }

  /** Get a raw void pointer to array data. Used by reader/writers.
   * You probably don't want this, see the example algorithm.
   */
  virtual void *
  getRawDataPointer() override
  {
    return myStorage.data();
  }

protected:

  /** Boost array we wrap */
  boost::multi_array<C, N> myStorage;
};

class Arrays : public Data {
public:
  // ----------------------------------------------------------
  // Convenience routines to keep students from doing too much templating...
  // Having separate class allows creation without messing with template arguments

  /** Convenience for returning a new 1d float array to auto */
  static std::shared_ptr<Array<float, 1> >
  CreateFloat1D(size_t x);

  /** Convenience for returning a new 1d int array to auto */
  static std::shared_ptr<Array<int, 1> >
  CreateInt1D(size_t x);

  /** Convenience for returning a new 2d float array to auto */
  static std::shared_ptr<Array<float, 2> >
  CreateFloat2D(size_t x, size_t y);

  /** Convenience for returning a new 1d float array to auto */
  static std::shared_ptr<Array<int, 2> >
  CreateInt2D(size_t x, size_t y);

  /** Convenience for returning a new 3d float array to auto */
  static std::shared_ptr<Array<float, 3> >
  CreateFloat3D(size_t x, size_t y, size_t z);

  /** Convenience for returning a new 3d int array to auto */
  static std::shared_ptr<Array<int, 3> >
  CreateInt3D(size_t x, size_t y, size_t z);
};
}
