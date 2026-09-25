#pragma once

#include <rArraySampler.h>
#include <memory>

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
  NearestNeighbor() : ArraySampler(){ }

  /** Introduce to factory */
  static void
  introduceSelf();

  /** Get help for us */
  virtual std::string
  getHelpString() override;

  // ----------------------------------------------------------------
  // The Template Trampoline
  // ----------------------------------------------------------------
  template <typename BndX, typename BndY, typename MapperType>
  void
  applySampler(std::shared_ptr<Array<float, 2> > src,
    std::shared_ptr<Array<float, 2> >            dst,
    const MapperType                             & mapper) const
  {
    struct CoreSampler {
      const boost::multi_array<float, 2> & srcData;
      const int                          srcW, srcH;

      inline float
      sample(float u, float v) const
      {
        // Round to nearest integer index
        int i = static_cast<int>(u + 0.5f);
        int j = static_cast<int>(v + 0.5f);

        // Resolve boundaries
        if (!BndX::resolve(i, srcW) || !BndY::resolve(j, srcH)) {
          return rapio::Constants::DataUnavailable;
        }
        return srcData[i][j];
      }
    };

    CoreSampler sampler{ src->ref(), static_cast<int>(src->getX()), static_cast<int>(src->getY()) };

    executeBulkResample(sampler, src, dst, mapper);
  }
};
} // namespace rapio
