#pragma once

#include "rConstants.h"

#include <memory>
#include <iostream>

#include "rBOOST.h"

BOOST_WRAP_PUSH
#include <boost/multi_array.hpp>
BOOST_WRAP_POP

namespace rapio {
// Define the ArrayFloat1DRef, ArrayFloat1DPtr, etc. that are types hiding the boost:multi_array
// in case we ever swap it with another array system, this will prevent algorithms
// from having to change code if that happens.
#define DeclareArrayRefForD(TYPESTRING, TYPE, DIMENSION) \
  using Array ## TYPESTRING ## DIMENSION ## DRef = boost::multi_array<TYPE, DIMENSION>&; \
  using Array ## TYPESTRING ## DIMENSION ## DPtr = boost::multi_array<TYPE, DIMENSION> *;

#define DeclareArrayRefs(TYPESTRING, TYPE) \
  DeclareArrayRefForD(TYPESTRING, TYPE, 1) \
  DeclareArrayRefForD(TYPESTRING, TYPE, 2) \
  DeclareArrayRefForD(TYPESTRING, TYPE, 3)

DeclareArrayRefs(Byte, int8_t)
DeclareArrayRefs(Short, short)
DeclareArrayRefs(Int, int)
DeclareArrayRefs(Float, float)
DeclareArrayRefs(Double, double)

/* Base no-template typeless interface class
 * This allows some generic typeless access and convenient
 * generic vector pointer storage say in DataGrid
 * @ingroup rapio_data
 * @brief Internal class storing an Array non-templated base
 */
class ArrayBase
{
public:

  /** Create array using an initializer list for dimension sizes */
  ArrayBase(std::initializer_list<size_t> dims) : myDims(dims)
  { }

  /** Create array from vector of dimension sizes */
  ArrayBase(const std::vector<size_t>& dims) : myDims(dims)
  { }

  /** Clone this array at a high level where we don't care about its specialization. */
  virtual std::shared_ptr<ArrayBase>
  CloneBase() = 0;

  /** Stolen from the template */
  size_t
  getNumDimensions()
  {
    return myDims.size();
  }

  /** Resize using an initialize list of dimensions */
  void
  resize(std::initializer_list<size_t> dims)
  {
    // Convert initializer list to vector and call the main resize method
    resize(std::vector<size_t>(dims));
  }

  /** Resize using a vector list of dimensions */
  virtual void
  resize(const std::vector<size_t>& dims)
  {
    myDims = dims;
  }

  /** Get a vector of the sizes for each dimension */
  const std::vector<size_t>&
  getSizes() const
  {
    return myDims;
  }

  /** Convenience to get dim zero, 'X' */
  size_t
  getX()
  {
    return ((myDims.size() > 0) ? myDims[0] : 0);
  }

  /** Convenience to get dim one, 'Y' */
  size_t
  getY()
  {
    return ((myDims.size() > 1) ? myDims[1] : 0);
  }

  /** Convenience to get dim two, 'Z' */
  size_t
  getZ()
  {
    return ((myDims.size() > 2) ? myDims[2] : 0);
  }

  /** Convenience to get dim four, 'K' */
  size_t
  getK()
  {
    return ((myDims.size() > 3) ? myDims[3] : 0);
  }

  /** Return a raw data pointer if possible */
  virtual void *
  getRawDataPointer() = 0;

  /** Convenience print of Array.  Has to print internal to get the templated method */
  virtual void
  printArray(std::ostream& out = std::cout, const std::string& indent = "    ", const std::string& divider = ", ",
    size_t wrap = 9) = 0;

protected:
  /** Vector of sizes for each dimension */
  std::vector<size_t> myDims;
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
  Array(std::initializer_list<size_t> dims) : ArrayBase(dims), myStorage(dims)
  { }

  /** Create array from vector of dimension sizes */
  Array(const std::vector<size_t>& dims) : ArrayBase(dims), myStorage(dims)
  { }

  /** Fill array with given value, requires storage knowledge  */
  void
  fill(C value)
  {
    auto a_ref = refAs1D();

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

  /** Resize using a vector list of dimensions */
  virtual void
  resize(const std::vector<size_t>& dims) override
  {
    myDims = dims;
    myStorage.resize(myDims);
  }

  /** Convenience print of Array.  Has to print internal to get the templated method */
  virtual void
  printArray(std::ostream& out = std::cout, const std::string& indent = "    ", const std::string& divider = ", ",
    size_t wrap = 9) override
  {
    auto a_ref = refAs1D();

    size_t counter = 0; // wrapping by item count which is 'ok'

    out << indent;
    for (auto i = a_ref.begin(); i != a_ref.end(); ++i) {
      // Need template specialization here, so 'we' have to do the print
      C& z = *i;
      // Netcdf ndump prints as unsigned int, we'll copy.
      // We 'could' add a flag to toggle say character vs int output
      if (std::is_same<C, int8_t>::value) {
        out << (int) (z);
      } else {
        out << z;
      }
      if (std::next(i) != a_ref.end()) {
        out << divider;
      }
      if (counter++ > wrap) {
        counter = 0;
        out << "\n" << indent;
      }
    }
  };

  /** Standard stream for this..wraps to printArray default */
  template <typename U, size_t P>
  friend std::ostream&
  operator << (std::ostream& os, const Array<U, P>& obj);

  /** Get a reference to raw array for iteration */
  boost::multi_array<C, N>&
  ref()
  {
    return myStorage;
  }

  /** Get a pointer to raw array*/
  boost::multi_array<C, N> *
  ptr()
  {
    return &myStorage;
  }

  /** Get a reference to raw array as a forced 1D array.
   * FIXME: Could use _ref in the general ref() above */
  boost::multi_array_ref<C, 1>
  refAs1D()
  {
    auto& dd = ref();

    return (boost::multi_array_ref<C, 1>(dd.data(), boost::extents[dd.num_elements()]));
  }

  /** Get a raw void pointer to array data. Used by reader/writers.
   * You probably don't want this, see the example algorithm.
   */
  virtual void *
  getRawDataPointer() override
  {
    return myStorage.data();
  }

  /** Clone this array at a high level where we don't care about its specialization.
   * This is useful in generic lists of different array types */
  virtual std::shared_ptr<ArrayBase>
  CloneBase() override
  {
    return Clone();
  }

  /** Clone off specialized copy.  We must know the type of Array we have */
  std::shared_ptr<Array<C, N> >
  Clone()
  {
    // Copy the contents of the original multi_array to the clone
    auto clone = std::make_shared<Array<C, N> >(myDims);

    std::copy(myStorage.data(), myStorage.data() + myStorage.num_elements(), clone->myStorage.data());

    return clone;
  }

protected:

  /** Boost array we wrap */
  boost::multi_array<C, N> myStorage;
};

template <typename U, size_t P>
std::ostream&
operator << (std::ostream& os, Array<U, P>& obj)
{
  obj.printArray(os);
  return os;
}

#define DeclareCreateArrayMethod1(TYPESTRING, TYPE) \
  static std::shared_ptr<Array<TYPE, 1> > \
  Create ## TYPESTRING ## 1 ## D(size_t x) \
  { \
    std::vector<size_t> dims = { x }; \
    std::shared_ptr<Array<TYPE, 1> > a = \
      std::make_shared<Array<TYPE, 1> >(dims); \
    return a; \
  }

#define DeclareCreateArrayMethod2(TYPESTRING, TYPE) \
  static std::shared_ptr<Array<TYPE, 2> > \
  Create ## TYPESTRING ## 2 ## D(size_t x, size_t y) \
  { \
    std::vector<size_t> dims = { x, y }; \
    std::shared_ptr<Array<TYPE, 2> > a = \
      std::make_shared<Array<TYPE, 2> >(dims); \
    return a; \
  }

#define DeclareCreateArrayMethod3(TYPESTRING, TYPE) \
  static std::shared_ptr<Array<TYPE, 3> > \
  Create ## TYPESTRING ## 3 ## D(size_t x, size_t y, size_t z) \
  { \
    std::vector<size_t> dims = { x, y, z }; \
    std::shared_ptr<Array<TYPE, 3> > a = \
      std::make_shared<Array<TYPE, 3> >(dims); \
    return a; \
  }

/** Declare up to 3D array access, haven't seen a need for anything higher 'yet'.
 * Make sure to sync these with the DeclareArrayFactoryMethods in the .cc  */
#define DeclareCreateArrayMethods(TYPESTRING, TYPE) \
  DeclareCreateArrayMethod1(TYPESTRING, TYPE) \
  DeclareCreateArrayMethod2(TYPESTRING, TYPE) \
  DeclareCreateArrayMethod3(TYPESTRING, TYPE)

/**
 *
 * @ingroup rapio_utility
 * @brief Utility class for static Array routiness.
 */
class Arrays {
public:
  // ----------------------------------------------------------
  // Convenience routines to keep students from doing too much templating...
  // Having separate class allows creation without messing with template arguments

  /** Functions for reference char/Byte arrays of 8 bit -128 to 127.
   * CreateByte1D, CreateByte2D, CreateByte3D */
  DeclareCreateArrayMethods(Byte, int8_t);

  /** Functions for reference short arrays of 16 bit
   * CreateShort1D, CreateShort2D, CreateShort3D */
  DeclareCreateArrayMethods(Short, short);

  /** Functions for reference int arrays of 32 bit
   * CreateInt1D, CreateInt2D, CreateInt3D */
  DeclareCreateArrayMethods(Int, int);

  /** Functions for reference float arrays of 32 bit
   * CreateFloat1D, CreateFloat2D, CreateFloat3D */
  DeclareCreateArrayMethods(Float, float);

  /** Functions for referencing double arrays of 64 bit
   * CreateDouble1D, CreateDouble2D, CreateDouble3D */
  DeclareCreateArrayMethods(Double, double);
};
}
