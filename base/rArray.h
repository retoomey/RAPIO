#pragma once

#include "rData.h"
#include "rNamedAny.h"
#include "rConstants.h"

#include <memory>
#include <iostream>

#include <boost/multi_array.hpp>

namespace rapio {
/*
 #ifdef BOOST_ARRAY
 *
 # define RAPIO_1DF boost::multi_array<float, 1>
 # define RAPIO_1DI boost::multi_array<int, 1>
 #
 # define RAPIO_2DF boost::multi_array<float, 2>
 #
 # define RAPIO_DIM1(x)       boost::extents[x]
 # define RAPIO_DIM2(x, y)    boost::extents[x][y]
 # define RAPIO_DIM3(x, y, z) boost::extents[x][y][z]
 #
 #else // ifdef BOOST_ARRAY
 #
 # define RAPIO_1DF DataStore<float>
 # define RAPIO_1DI DataStore<int>
 # define RAPIO_2DF DataStore2D<float>
 #
 # define RAPIO_DIM1(x)       x
 # define RAPIO_DIM2(x, y)    x, y
 # define RAPIO_DIM3(x, y, z) x, y, z
 #
 #endif // ifdef BOOST_ARRAY
 */

/* Storage of an array API.  Wraps another storage system.
 * We do this to hide the details of whatever storage
 * we're using so we can swap libraries if needed.
 *
 * Array<float, 2>({100,100}, 2.3f);
 *
 * @author Robert Toomey
 */
template <typename C, size_t N> class Array : public Any
{
public:

  // Need at least one dimension passed in for a 1D array, rest are optional...
  // Array<float>(100,50) 2D array  size 100x50
  // Array<double>(100) 1D array size 100

  //
  // auto array = Array::make<float>({100,100}) ?
  // // I could use a template for the class?
  // Array<float, 2>  Lock to 2 dimensions like boost...
  //
  //
  /** Array<float>({100,100}) or Array<float>({50,23}, 5.0f); */
  Array(std::initializer_list<size_t> dims, C value = 0)
  {
    for (auto x:dims) {
      myDims.push_back(x);
    }
    syncToDims();
  }

  /** Create array from array of vectors */
  Array(std::vector<size_t> dims, C value = 0)
  {
    myDims = dims;
    syncToDims();
  }

  /** Create the array for current dimensions */
  bool
  syncToDims()
  {
    std::shared_ptr<boost::multi_array<C, N> > myStorage =
      std::make_shared<boost::multi_array<C, N> >(
      myDims);
    //   boost::extents(myDims));
    //  auto& dd= *myStorage; // should be pointer right?
    //  LogSevere("Create pointer is " << (void*)(&(dd)) << "\n");

    set(myStorage);
    //  auto back = getSP<boost::multi_array<C, N> >();
    //  auto& dd2= *back; // should be pointer right?
    //  LogSevere("Back  pointer is " << (void*)(&(dd2)) << "\n");

    return true;
  }

  /** Get a vector of the sizes for each dimension */
  const std::vector<size_t>&
  getSizes() const
  {
    return myDims;
  }

  void
  dump()
  {
    auto sp  = data();
    auto& dd = *sp; // auto without & is copying...bleh

    LogSevere("Dumping array for  " << (void *) (&(dd)) << "\n");

    // LogSevere("THE ADDRESS " << (void*)(&(*myData)) << "\n");
    for (size_t x = 0; x < myDims[0]; x++) {
      for (size_t y = 0; y < myDims[1]; y++) {
        std::cout << dd[x][y] << ", ";
      }
      std::cout << "\n";
    }
    std::cout << "\n";
  }

  /** Get the raw array for iteration  */
  std::shared_ptr<boost::multi_array<C, N> >
  data()
  {
    auto data = getSP<boost::multi_array<C, N> >();

    return data;
  }

  /** Get a raw void pointer to array data. Used by reader/writers.
   * You probably don't want this, see the example algorithm.
   * FIXME: Maybe better as an optional */
  void *
  getRawDataPointer()
  {
    auto data = getSP<boost::multi_array<C, N> >()->data();

    return data;
  }

protected:
  /** Vector of sizes for each dimension */
  std::vector<size_t> myDims;
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
