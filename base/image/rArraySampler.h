#pragma once
#include <rArray.h>
#include <rArrayStage.h>
#include <cmath>

namespace rapio {
// ----------------------------------------------------------------
// High-Performance Template Wrapper
// We use a template vs virtual lookup for speed.  It's quite a bit,
// testing 15x increase for basic filters/resampling
// ----------------------------------------------------------------
template <typename SamplerType, typename MapperType>
void
executeBulkResample(const SamplerType      & sampler,
  std::shared_ptr<rapio::Array<float, 2> > src,
  std::shared_ptr<rapio::Array<float, 2> > dst,
  const MapperType                         & mapper)
{
  if (!src || !dst) { return; }
  auto& dstData = dst->ref();
  size_t dstW   = dst->getX();
  size_t dstH   = dst->getY();

  for (size_t i = 0; i < dstW; ++i) {
    float u = mapper.mapY(i);
    for (size_t j = 0; j < dstH; ++j) {
      float v      = mapper.mapX(j);
      float outVal = sampler.sample(u, v);
      if (outVal != rapio::Constants::DataUnavailable) {
        dstData[i][j] = outVal;
      }
    }
  }
}

/* The ArraySampler classes resample from one array to another
 *
 * @author Robert Toomey
 * @ingroup rapio_image
 * @brief Base class for array sampling.
 */
class ArraySampler : public ArrayStage {
public:

  /**
   * @brief Virtual destructor to ensure proper cleanup of derived sampling classes.
   */
  virtual
  ~ArraySampler() = default;
};
} // namespace rapio
