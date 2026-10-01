#pragma once

#include <rArraySampler.h>
#include <rConstants.h>
#include <memory>
#include <string>

namespace rapio {
/**
 * @brief Perform nearest-neighbor interpolation on a 2D grid.
 * @ingroup rapio_image
 *
 * This class maps a destination pixel to the single closest source pixel.
 * It is the fastest sampling method and preserves exact original data values
 * without introducing new interpolated values (crucial for categorical data).
 */
class NearestNeighbor : public ArraySampler {
public:
  /** Create nearest neighbor sampler */
  NearestNeighbor() = default;

  /** Introduce to factory */
  static void
  introduceSelf();

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
};
} // namespace rapio
