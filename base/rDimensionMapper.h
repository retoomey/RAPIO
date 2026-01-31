#pragma once

#include <vector>

namespace rapio {
/** A class that maps N dimensions to a single dimension.
 * @author Robert Toomey
 * @ingroup rapio_utility
 * @brief Class for flattening N dimensions to a single one.
 */
class DimensionMapper {
public:

  /** Empty STL dimension mapper */
  DimensionMapper(){ }

  /** Create a dimension mapper from a vector of max dimensions */
  DimensionMapper(std::vector<size_t> dimensions) :
    myDimensions(dimensions), myStrides(std::vector<size_t>(dimensions.size(), 1))
  {
    calculateStrides();
  }

  /** Single dimension */
  DimensionMapper(size_t dimension) :
    myDimensions({ dimension }), myStrides(std::vector<size_t>(1, 1))
  {
    calculateStrides();
  }

  /** Calculate total size required to store all dimensions given.
   * Used during creation to make any linear non-sparse storage.
   * This is just multiplication of all values */
  static size_t
  calculateSize(const std::vector<size_t>& values)
  {
    size_t total = 1;

    for (size_t value: values) {
      total *= value;
    }
    return total;
  }

  /* (AI) Cache strides for our dimensions, this reduces the multiplications/additions
   *  when converting dimension coordinates into the linear index */
  void
  calculateStrides()
  {
    for (int i = myDimensions.size() - 2; i >= 0; --i) {
      myStrides[i] = myStrides[i + 1] * myDimensions[i + 1];
    }
  }

  /** (AI) Calculate index in the dimension space, no checking */
  size_t
  getIndex(std::vector<size_t> indices)
  {
    size_t index = 0;

    for (int i = myDimensions.size() - 1; i >= 0; --i) { // match w2merger ordering
      index += indices[i] * myStrides[i];
    }
    return index;
  }

  /** Quick index for 3D when you know you have 3 dimensions.
   * This is basically a collapsed form of the general getIndex.
   */
  inline size_t
  getIndex3D(size_t x, size_t y, size_t z)
  {
    return ((z * myStrides[2]) + (y * myStrides[1]) + (x * myStrides[0]));
  }

  /** Keeping for moment, this is how w2merger calculates its
   * linear index.  We made these match for consistency:
   * getOldIndex({x,y,z}) == getIndex({x,y,z}) for all values.
   */
  size_t
  getOldIndex(std::vector<size_t> i)
  {
    size_t horsize = myDimensions[1] * myDimensions[2];
    size_t zsize   = myDimensions[2];

    return (i[0] * horsize + i[1] * zsize + i[2]);
  }

  /** Return list of dimension sizes */
  std::vector<size_t>
  getDimensions()
  {
    return myDimensions;
  }

  /** Return list of strides for debugging */
  std::vector<size_t>
  getStrides()
  {
    return myStrides;
  }

protected:

  /** The number in each dimension */
  std::vector<size_t> myDimensions;

  /** Strides for each dimension.  Cache for calculating index */
  std::vector<size_t> myStrides;
};
}
