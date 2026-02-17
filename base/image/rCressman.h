#pragma once

#include <rArraySampler.h>

#include <memory>

namespace rapio {
/**
 * @brief Perform Cressman interpolation on a 2D grid.
 * @ingroup rapio_image
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
class Cressman : public ArraySampler {
public:

  /** Create a Cressman sampler */
  Cressman(size_t width = 3, size_t height = 3
  ) : ArraySampler(), myWidth(width), myHeight(height)
  { }

  /** Introduce to factory */
  static void
  introduceSelf();

  /** Parse string options in the factory */
  virtual bool
  parseOptions(const std::vector<std::string>& part,
    std::shared_ptr<ArrayAlgorithm>          upstream) override;

  /** Get help for us */
  virtual std::string
  getHelpString() override;

  /** Get value in source at virtual index location */
  virtual bool
  sampleAt(float inI, float inJ, float& out) override;

protected:

  /** Width for the Cressman interpolation submatrix */
  size_t myWidth;

  /** Height for the Cressman interpolation submatrix */
  size_t myHeight;
};
}
