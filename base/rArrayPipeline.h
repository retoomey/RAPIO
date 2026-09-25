#pragma once

#include <rArray.h>

#include <rArrayBoundary.h>
#include <rArrayFilter.h>
#include <rNearestNeighbor.h>
#include <rBilinear.h>
#include <rCressman.h>

#include <memory>
#include <vector>
#include <string>

namespace rapio {
// A simple 1:1 mapper for discrete filters
struct IdentityMapper {
  inline float
  mapY(int destI) const { return static_cast<float>(destI); }

  inline float
  mapX(int destJ) const { return static_cast<float>(destJ); }
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
  template <typename MapperType>
  void
  remap(std::shared_ptr<Array<float, 2> > src,
    std::shared_ptr<Array<float, 2> >     dst,
    const MapperType                      & mapper)
  {
    if (!src || !dst) { return; }
    dispatchSampler(src, dst, mapper);
    executeFiltersPingPong(dst);
  }

  // 2. Filter-Only Pipeline (Out-of-place)
  void
  process(std::shared_ptr<Array<float, 2> > src,
    std::shared_ptr<Array<float, 2> >       dst);

  // 3. Filter-Only Pipeline (In-place)
  void
  processInPlace(std::shared_ptr<Array<float, 2> > data);

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

  template <typename BndX, typename BndY, typename MapperType>
  void
  applySamplerT(std::shared_ptr<Array<float, 2> > src,
    std::shared_ptr<Array<float, 2> >             dst,
    const MapperType                              & mapper)
  {
    ArraySampler * rawSampler = mySampler.get();

    // Bypass RTTI boundary failures by relying on the parsed string type
    // and forcing a static_cast to the known concrete type.
    if ((mySamplerType == "cressman") && rawSampler) {
      static_cast<Cressman *>(rawSampler)->applySampler<BndX, BndY>(src, dst, mapper);
    } else if ((mySamplerType == "bilinear") && rawSampler) {
      static_cast<Bilinear *>(rawSampler)->applySampler<BndX, BndY>(src, dst, mapper);
    } else if ((mySamplerType == "nearest") && rawSampler) {
      static_cast<NearestNeighbor *>(rawSampler)->applySampler<BndX, BndY>(src, dst, mapper);
    } else {
      NearestNeighbor nn;
      nn.applySampler<BndX, BndY>(src, dst, mapper);
    }
  }

  template <typename MapperType>
  void
  dispatchSampler(std::shared_ptr<Array<float, 2> > src,
    std::shared_ptr<Array<float, 2> >               dst,
    const MapperType                                & mapper)
  {
    Boundary bx = mySampler ? mySampler->getXBoundary() : Boundary::None;
    Boundary by = mySampler ? mySampler->getYBoundary() : Boundary::None;

    RAPIO_DISPATCH_BOUNDARIES(bx, by, applySamplerT, src, dst, mapper);
  }

  void
  executeFiltersPingPong(std::shared_ptr<Array<float, 2> > target);

  std::shared_ptr<ArraySampler> mySampler;
  std::string mySamplerType = "";
  std::vector<std::shared_ptr<ArrayFilter> > myFilters;
};
} // namespace rapio
