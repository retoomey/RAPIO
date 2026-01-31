#pragma once

#include <rArray.h>

#include <memory>

namespace rapio {
/** ArrayAlgorithm
 *
 * Process on an Array
 *
 * Original ideas from Lak using Array vs the MRMS Image class.
 * We have an advantage in RAPIO since our data arrays are
 * all Array classes.
 *
 * To start we have filter operations such as nearest neighbor,
 * cressman, etc.
 * FIXME: Improve API over time here, add ability.
 * FIXME: Think we could wrap opencv optionally for some things
 *
 * @author Robert Toomey
 * @ingroup rapio_utility
 * @brief Algorithm API for processing our Array classes.
 */
class ArrayAlgorithm {
public:

  /** Pass in two array for input/output (copy)
   * Currently we're starting off with two 2D arrays.*/
  ArrayAlgorithm(size_t width = 1, size_t height = 1) :
    myArrayIn(nullptr), myArrayOut(nullptr),
    myRefIn(nullptr), myRefOut(nullptr), myMaxI(0), myMaxJ(0),
    myWidth(width), myHeight(height)
  { }

  /** Apply filter to out indexes (copy from source using our technique) */
  virtual bool
  remap(float inI, float inJ, float& out) = 0;

  /** Set source array.  Needs to be called before direct remap */
  void
  setSource(std::shared_ptr<Array<float, 2> > in)
  {
    myArrayIn = in;
    auto& i = *myArrayIn;

    myRefIn = i.ptr();
    myMaxI  = i.getX();
    myMaxJ  = i.getY();
    // myWidth, myHeight in constructor (param for now)
  }

  /** Set output array. */
  void
  setOutput(std::shared_ptr<Array<float, 2> > out)
  {
    myArrayOut = out;
    myRefOut   = myArrayOut->ptr();
  }

public:

  /** Array passed in shared, if any.  Shared ensures scope. */
  std::shared_ptr<Array<float, 2> > myArrayIn;

  /** Array out shared, if any.  Shared ensures scope. */
  std::shared_ptr<Array<float, 2> > myArrayOut;

  /** Keep input array reference for speed */
  boost::multi_array<float, 2> * myRefIn;

  /** Keep output array reference for speed */
  boost::multi_array<float, 2> * myRefOut;

  /** Max dimension for speed in remap */
  size_t myMaxI;

  /** Max dimension for speed in remap */
  size_t myMaxJ;

  /** Width for the submatrix (if used) */
  size_t myWidth;

  /** Height for the submatrix (if used) */
  size_t myHeight;
};

/**
 * @brief Perform nearest neighbor sampling on a 2D grid.
 * @ingroup rapio_utility
 **/
class NearestNeighbor : public ArrayAlgorithm {
public:

  /** create nearest neighbor from source to destination array */
  NearestNeighbor() : ArrayAlgorithm()
  { }

  /** remap the remap */
  virtual bool
  remap(float inI, float inJ, float& out) override;
};

/**
 * @brief Perform Cressman interpolation on a 2D grid.
 * @ingroup rapio_utility
 *
 * This class interpolates a value at a given point (x, y) using Cressman interpolation
 * from the surrounding grid points within an N x N neighborhood.
 *
 * @note
 * Example of Cressman interpolation:
 * @code
 * // Grid values:
 * //  f(1, 1) = 10, f(2, 1) = 20
 * //  f(1, 2) = 30, f(2, 2) = 40
 * // Interpolation point: (1.5, 1.5)
 * // Neighborhood size: N = 2
 * //
 * // Euclidean distances:
 * //  d(1,1) = sqrt((1.5 - 1)^2 + (1.5 - 1)^2) ≈ 0.707
 * //  d(2,1) = sqrt((1.5 - 2)^2 + (1.5 - 1)^2) ≈ 0.707
 * //  d(1,2) = sqrt((1.5 - 1)^2 + (1.5 - 2)^2) ≈ 0.707
 * //  d(2,2) = sqrt((1.5 - 2)^2 + (1.5 - 2)^2) ≈ 0.707
 * //
 * // Weights:
 * //  Weight for (1, 1) = 1 / 0.707 ≈ 1.414
 * //  Weight for (2, 1) = 1 / 0.707 ≈ 1.414
 * //  Weight for (1, 2) = 1 / 0.707 ≈ 1.414
 * //  Weight for (2, 2) = 1 / 0.707 ≈ 1.414
 * //
 * // Interpolated value:
 * //  f(1.5, 1.5) = (10 * 1.414 + 20 * 1.414 + 30 * 1.414 + 40 * 1.414) /
 * //                (1.414 + 1.414 + 1.414 + 1.414) = 25
 * @endcode
 */
class Cressman : public ArrayAlgorithm {
public:

  /** Create nearest neighbor from source to destination array */
  Cressman(size_t width = 3, size_t height = 3
  ) : ArrayAlgorithm(width, height)
  { }

  /** Remap grid location to output */
  virtual bool
  remap(float inI, float inJ, float& out) override;
};

/*
 * @brief Perform bilinear interpolation on a 2D grid.
 * @ingroup rapio_utility
 *
 * Interpolates a value at a given point (x, y) using bilinear interpolation
 * from the surrounding grid points.
 *
 * @note
 * Example of bilinear interpolation:
 * @code
 * // Grid values:
 * //  f(1, 1) = 10, f(2, 1) = 20
 * //  f(1, 2) = 30, f(2, 2) = 40
 * // Interpolation point: (1.5, 1.5)
 * //
 * // dx = 1.5 - 1 = 0.5
 * // dy = 1.5 - 1 = 0.5
 * //
 * // Weights:
 * //  Weight for (1, 1) = (1 - 0.5) * (1 - 0.5) = 0.25
 * //  Weight for (2, 1) = 0.5 * (1 - 0.5) = 0.25
 * //  Weight for (1, 2) = (1 - 0.5) * 0.5 = 0.25
 * //  Weight for (2, 2) = 0.5 * 0.5 = 0.25
 * //
 * // Interpolated value:
 * //  f(1.5, 1.5) = 10 * 0.25 + 20 * 0.25 + 30 * 0.25 + 40 * 0.25 = 25
 * @endcode
 */
class Bilinear : public ArrayAlgorithm {
public:

  /** Create nearest neighbor from source to destination array */
  Bilinear(size_t width = 3, size_t height = 3
  ) : ArrayAlgorithm(width, height)
  { }

  /** Remap grid location to output */
  virtual bool
  remap(float inI, float inJ, float& out) override;
};
}
