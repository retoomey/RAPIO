#pragma once

#include <rArraySampler.h>
#include <rConstants.h>
#include <memory>
#include <cmath>
#include <limits>
#include <string>

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
  Cressman(size_t width = 3, size_t height = 3)
    : ArraySampler(), myWidth(width), myHeight(height){ }

  /** Introduce to factory */
  static void
  introduceSelf();

  /** Parse string options in the factory */
  bool
  parseOptions(const std::string& params) override;

  /** Get help for us */
  std::string
  getHelpString() override;

  /** Capability query */
  bool
  supportsDimensions(size_t dims) const override
  {
    return (dims == 2);
  }

  /** Unified 2D resampling endpoint */
  void
  remap2D(std::shared_ptr<Array<float, 2> >& src,
    std::shared_ptr<Array<float, 2> >      & dst,
    const ArrayMapper                      & mapper) override;

private:
  template <typename BndX, typename BndY>
  void
  applyFilter(std::shared_ptr<Array<float, 2> >& src,
    std::shared_ptr<Array<float, 2> >          & dst,
    const ArrayMapper                          & mapper) const;

protected:
  /** Width for the Cressman interpolation submatrix */
  size_t myWidth;

  /** Height for the Cressman interpolation submatrix */
  size_t myHeight;
};
} // namespace rapio
