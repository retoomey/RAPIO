#pragma once

#include <rArraySampler.h>
#include <memory>
#include <cmath>
#include <limits>

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
    : ArraySampler(), myWidth(width), myHeight(height)
  { }

  /** Introduce to factory */
  static void
  introduceSelf();

  /** Parse string options in the factory */
  virtual bool
  parseOptions(const std::vector<std::string>& part) override;

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
    // The nested high-performance struct
    struct CoreSampler {
      const boost::multi_array<float, 2> & srcData;
      const int                          srcW, srcH;
      const float                        radX, radY, invRadX2, invRadY2;

      inline float
      sample(float u, float v) const
      {
        int startX = static_cast<int>(std::floor(u - radX));
        int endX   = static_cast<int>(std::ceil(u + radX));
        int startY = static_cast<int>(std::floor(v - radY));
        int endY   = static_cast<int>(std::ceil(v + radY));

        // Sum up all the weights for each sample
        float sumWt       = 0.0f;
        float sumVal      = 0.0f;
        float currentMask = rapio::Constants::DataUnavailable;

        for (int i = startX; i <= endX; ++i) {
          // 1. Geometry (Unwrapped): Where is the point spatially?
          // 2. Topology (Wrapped): Where is the data in memory?
          int memX = i;

          // Resolve Boundary (modifies memX if wrapping)
          // If the lat is invalid, all the lons in the row will be invalid
          if (!BndX::resolve(memX, srcW)) { continue; }

          // DISTANCE: Use unwrapped spatial index
          // If we used memX here, wrapping would break the distance logic
          float dx         = static_cast<float>(i) - u;
          float dx2_scaled = (dx * dx) * invRadX2;
          if (dx2_scaled >= 1.0f) { continue; }

          // For the change in lon row...
          for (int j = startY; j <= endY; ++j) {
            // Geometry vs Topology for J dimension
            int memY = j;

            // ...if lon valid, check if a good value
            // Resolve Boundary
            if (!BndY::resolve(memY, srcH)) { continue; }

            float dy = static_cast<float>(j) - v;
            float d2 = dx2_scaled + (dy * dy) * invRadY2;

            if (d2 <= 1.0f) {
              float val = srcData[memX][memY];

              // ...if the data value good, add weight to total...
              if (rapio::Constants::isGood(val)) {
                // If the distance is extremely small, use the cell exact value
                // to avoid division by zero
                // This also passes on mask when close to a true cell location
                if (d2 < std::numeric_limits<float>::epsilon()) {
                  return val;
                }

                float wt = (1.0f - d2) / (1.0f + d2);
                sumWt  += wt;
                sumVal += val * wt;
              } else {
                // If any value in our matrix sampling is missing, we'll use that as a mask
                // if there are no good values to interpolate.  Should work
                if (val == rapio::Constants::MissingData) {
                  currentMask = rapio::Constants::MissingData;
                }
              }
            }
          } // End lon row
        }   // End lat column

        if (sumWt > 0.0f) {
          return sumVal / sumWt;
        }
        return currentMask;
      } // sample
    };

    CoreSampler sampler{
      src->ref(), static_cast<int>(src->getX()), static_cast<int>(src->getY()),
      static_cast<float>(myWidth) / 2.0f, static_cast<float>(myHeight) / 2.0f,
      1.0f / std::pow(myWidth / 2.0f, 2.0f), 1.0f / std::pow(myHeight / 2.0f, 2.0f)
    };

    executeBulkResample(sampler, src, dst, mapper);
  } // applySampler

protected:
  /** Width for the Cressman interpolation submatrix */
  size_t myWidth;

  /** Height for the Cressman interpolation submatrix */
  size_t myHeight;
};
}
