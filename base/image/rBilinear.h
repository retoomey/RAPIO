#pragma once

#include <rArraySampler.h>

#include <memory>

namespace rapio {
/*
 * @brief Perform bilinear interpolation on a 2D grid.
 * @ingroup rapio_image
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
class Bilinear : public ArraySampler {
public:

  /** Create nearest neighbor from source to destination array */
  Bilinear(size_t width = 3, size_t height = 3
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

  /** Get value in source at index location */
  virtual bool
  sampleAt(float inI, float inJ, float& out) override;

protected:

  /** Width for the submatrix */
  size_t myWidth;

  /** Height for the submatrix */
  size_t myHeight;
};
}
