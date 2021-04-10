#pragma once

#include "rData.h"
#include "rNamedAny.h"
#include "rConstants.h"

#include <memory>
#include <iostream>

#include <boost/multi_array.hpp>

namespace rapio {
/* Storage of an array API.  Wraps another storage system.
 * We do this to hide the details of whatever storage
 * we're using so we can swap libraries if needed.
 *
 * Create directly:
 * Array<float, 2>({100,100}, 2.3f);
 *
 * @author Robert Toomey
 */
template <typename C, size_t N> class Array : public Data
{
public:

  /** Create array using an initializer list for dimension sizes */
  Array(std::initializer_list<size_t> dims, C value = 0)
  {
    for (auto x:dims) {
      myDims.push_back(x);
    }
    syncToDims();
  }

  /** Create array from vector of dimension sizes */
  Array(const std::vector<size_t>& dims, C value = 0) : myDims(dims)
  {
    syncToDims();
  }

  /** Resize using an initialize list of dimensions */
  void
  resize(std::initializer_list<size_t> dims)
  {
    myDims.clear();
    for (auto x:dims) {
      myDims.push_back(x);
    }
    syncToDims();
  }

  /** Resize using a vector list of dimensions */
  void
  resize(const std::vector<size_t>& dims)
  {
    myDims = dims;
    syncToDims();
  }

  /** Fill array with given value */
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
  bool
  syncToDims()
  {
    // Have to use resize on the current array in our class
    myStorage.resize(myDims);
    return true;
  }

  /** Get a vector of the sizes for each dimension */
  const std::vector<size_t>&
  getSizes() const
  {
    return myDims;
  }

  /** Dump entire array to std::cout.  Used for debugging */
  void
  dump()
  {
    auto& dd = ref();

    LogSevere("Dumping array for  " << (void *) (&(dd)) << "\n");

    for (size_t x = 0; x < myDims[0]; x++) {
      for (size_t y = 0; y < myDims[1]; y++) {
        std::cout << dd[x][y] << ", ";
      }
      std::cout << "\n";
    }
    std::cout << "\n";
  }

  /** Get a reference to raw array for iteration */
  boost::multi_array<C, N>&
  ref()
  {
    return myStorage;
  }

  /** Get a raw void pointer to array data. Used by reader/writers.
   * You probably don't want this, see the example algorithm.
   */
  void *
  getRawDataPointer()
  {
    return myStorage.data();
  }

protected:
  /** Vector of sizes for each dimension */
  std::vector<size_t> myDims;

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
  CreateFloat1D(size_t x, float v = Constants::MissingData);

  /** Convenience for returning a new 2d float array to auto */
  static std::shared_ptr<Array<float, 2> >
  CreateFloat2D(size_t x, size_t y, float v = Constants::MissingData);

  /** Convenience for returning a new 3d float array to auto */
  static std::shared_ptr<Array<float, 3> >
  CreateFloat3D(size_t x, size_t y, size_t z, float v = Constants::MissingData);
};
}
