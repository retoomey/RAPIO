#pragma once

#include <rArraySampler.h>
#include <memory>

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
    struct CoreSampler {
      const boost::multi_array<float, 2> & srcData;
      const int                          srcW, srcH;

      inline float
      sample(float u, float v) const
      {
        int iu = static_cast<int>(u);
        int iv = static_cast<int>(v);

        int i00_x = iu, i00_y = iv;
        int i10_x = iu + 1, i10_y = iv;
        int i01_x = iu, i01_y = iv + 1;
        int i11_x = iu + 1, i11_y = iv + 1;

        // Resolve boundaries for the 4 corners
        if (!BndX::resolve(i00_x, srcW) || !BndY::resolve(i00_y, srcH) ||
          !BndX::resolve(i10_x, srcW) || !BndY::resolve(i10_y, srcH) ||
          !BndX::resolve(i01_x, srcW) || !BndY::resolve(i01_y, srcH) ||
          !BndX::resolve(i11_x, srcW) || !BndY::resolve(i11_y, srcH))
        {
          return rapio::Constants::DataUnavailable;
        }

        float p00 = srcData[i00_x][i00_y];
        float p10 = srcData[i10_x][i10_y];
        float p01 = srcData[i01_x][i01_y];
        float p11 = srcData[i11_x][i11_y];

        // Fallback to nearest neighbor if any corner is missing/bad data
        if (!rapio::Constants::isGood(p00) || !rapio::Constants::isGood(p10) ||
          !rapio::Constants::isGood(p01) || !rapio::Constants::isGood(p11))
        {
          int i = static_cast<int>(u + 0.5f);
          int j = static_cast<int>(v + 0.5f);
          if (!BndX::resolve(i, srcW) || !BndY::resolve(j, srcH)) {
            return rapio::Constants::DataUnavailable;
          }
          return srcData[i][j];
        }

        float fx = u - static_cast<float>(iu);
        float fy = v - static_cast<float>(iv);
        float nx = 1.0f - fx;
        float ny = 1.0f - fy;

        // Standard bilinear interpolation math
        return p00 * (nx * ny) + p10 * (fx * ny) + p01 * (nx * fy) + p11 * (fx * fy);
      } // sample
    };

    CoreSampler sampler{ src->ref(), static_cast<int>(src->getX()), static_cast<int>(src->getY()) };

    executeBulkResample(sampler, src, dst, mapper);
  } // applySampler

protected:
  /** Width for the submatrix */
  size_t myWidth;

  /** Height for the submatrix */
  size_t myHeight;
};
} // namespace rapio
