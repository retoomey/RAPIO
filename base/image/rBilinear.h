#pragma once

#include <rArraySampler.h>
#include <rConstants.h>
#include <memory>
#include <string>

namespace rapio {
/**
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
  /** Create bilinear from source to destination array */
  Bilinear(size_t width = 3, size_t height = 3)
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
  /** Width for the submatrix (parsed but mathematically fixed to 2x2 for true bilinear) */
  size_t myWidth;

  /** Height for the submatrix (parsed but mathematically fixed to 2x2 for true bilinear) */
  size_t myHeight;
};
} // namespace rapio
