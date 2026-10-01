#pragma once
#include <rArray.h>
#include <rArrayStage.h>
#include <cmath>
#include <memory>
#include <vector>

namespace rapio {
/**
 * @brief Unified interface for mapping coordinates between geometric spaces.
 * Batched mapping functions eliminate virtual dispatch overhead in the inner loops.
 */
class ArrayMapper {
public:
  virtual
  ~ArrayMapper() = default;

  // Batch map an entire axis of coordinates in one virtual call.
  // The 'out' arrays must be pre-allocated to 'count' size.
  virtual void
  map1D(size_t destStart, size_t count, float * outU) const { }

  // 2D Mapping: Map destination indices to source coordinates (u, v)
  virtual void
  map2D_X(size_t destJ_start, size_t count, float * outV) const { }

  virtual void
  map2D_Y(size_t destI_start, size_t count, float * outU) const { }

  // 3D Mapping: Map destination indices to source coordinates (u, v, w)
  virtual void
  map3D_X(size_t destJ_start, size_t count, float * outV) const { }

  virtual void
  map3D_Y(size_t destI_start, size_t count, float * outU) const { }

  virtual void
  map3D_Z(size_t destK_start, size_t count, float * outW) const { }
};

/**
 * The ArraySampler classes resample from one array to another
 *
 * @author Robert Toomey
 * @ingroup rapio_image
 * @brief Base class for array sampling.
 */
class ArraySampler : public ArrayStage {
public:
  virtual
  ~ArraySampler() = default;

  /**
   * @brief Capability query: Does this sampler support 1D, 2D, or 3D data?
   */
  virtual bool
  supportsDimensions(size_t dims) const { return false; }

  /**
   * @brief 1D Resampling endpoint
   */
  virtual void remap1D(std::shared_ptr<Array<float, 1> >& src,
    std::shared_ptr<Array<float, 1> >                   & dst,
    const ArrayMapper                                   & mapper){ }

  /**
   * @brief 2D Resampling endpoint
   */
  virtual void remap2D(std::shared_ptr<Array<float, 2> >& src,
    std::shared_ptr<Array<float, 2> >                   & dst,
    const ArrayMapper                                   & mapper){ }

  /**
   * @brief 3D Resampling endpoint
   */
  virtual void remap3D(std::shared_ptr<Array<float, 3> >& src,
    std::shared_ptr<Array<float, 3> >                   & dst,
    const ArrayMapper                                   & mapper){ }

protected:
  void
  getMappedCoords2D(const ArrayMapper& mapper, size_t dstW, size_t dstH,
    std::vector<float>& uCoords, std::vector<float>& vCoords) const
  {
    uCoords.resize(dstW);
    vCoords.resize(dstH);
    mapper.map2D_Y(0, dstW, uCoords.data());
    mapper.map2D_X(0, dstH, vCoords.data());
  }
};
} // namespace rapio
