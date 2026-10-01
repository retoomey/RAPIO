#pragma once

#include <rArray.h>
#include <rArrayFilter.h>
#include <rArraySampler.h>

#include <memory>
#include <vector>
#include <string>

namespace rapio {
// A simple 1:1 mapper for discrete filters
struct IdentityMapper : public ArrayMapper {
  void
  map1D(size_t destStart, size_t count, float * outU) const override
  {
    for (size_t i = 0; i < count; ++i) { outU[i] = static_cast<float>(destStart + i); }
  }

  void
  map2D_X(size_t destJ_start, size_t count, float * outV) const override
  {
    for (size_t j = 0; j < count; ++j) { outV[j] = static_cast<float>(destJ_start + j); }
  }

  void
  map2D_Y(size_t destI_start, size_t count, float * outU) const override
  {
    for (size_t i = 0; i < count; ++i) { outU[i] = static_cast<float>(destI_start + i); }
  }

  void
  map3D_X(size_t destJ_start, size_t count, float * outV) const override
  {
    for (size_t j = 0; j < count; ++j) { outV[j] = static_cast<float>(destJ_start + j); }
  }

  void
  map3D_Y(size_t destI_start, size_t count, float * outU) const override
  {
    for (size_t i = 0; i < count; ++i) { outU[i] = static_cast<float>(destI_start + i); }
  }

  void
  map3D_Z(size_t destK_start, size_t count, float * outW) const override
  {
    for (size_t k = 0; k < count; ++k) { outW[k] = static_cast<float>(destK_start + k); }
  }
};

class ArrayPipeline {
public:
  ArrayPipeline() = default;
  static std::shared_ptr<ArrayPipeline>
  create(const std::string& config);

  /** Introduce all of the various filters */
  static void
  introduceSelf();

  /** Introduce help for samplers/filters */
  static std::string
  introduceHelp();

  // 1. Resample (+ Filter) Pipeline
  void
  remap(const std::shared_ptr<ArrayBase>& src,
    const std::shared_ptr<ArrayBase>    & dst,
    const ArrayMapper                   & mapper) const;

  // 2. Filter-Only Pipeline (Out-of-place)
  void
  process(const std::shared_ptr<ArrayBase>& src,
    const std::shared_ptr<ArrayBase>      & dst) const;

  // Convenience for std::vector calling (works for 1D stuff)
  void
  process(const std::vector<float>& src, std::vector<float>& dst) const;

  // 3. Filter-Only Pipeline (In-place)
  void
  processInPlace(const std::shared_ptr<ArrayBase>& data) const;

  void
  setBoundary(Boundary x, Boundary y)
  {
    if (mySampler) {
      mySampler->setBoundary(x, y);
    }
    for (auto& filter : myFilters) {
      filter->setBoundary(x, y);
    }
  }

private:
  void
  executeFiltersPingPong1D(const std::shared_ptr<Array<float, 1> >& target) const;
  void
  executeFiltersPingPong2D(const std::shared_ptr<Array<float, 2> >& target) const;
  void
  executeFiltersPingPong3D(const std::shared_ptr<Array<float, 3> >& target) const;

  std::shared_ptr<ArraySampler> mySampler;
  std::string mySamplerType = "";
  std::vector<std::shared_ptr<ArrayFilter> > myFilters;
};
} // namespace rapio
