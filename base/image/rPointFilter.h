#pragma once
#include <rArrayFilter.h>
#include <vector>

namespace rapio {
/** Point filters only change a point in the data without affecting
 * points around them.  So a threshold, etc.  We can optimize for
 * this type of filter. */
class PointFilter : public ArrayFilter {
public:
  virtual
  ~PointFilter() = default;

  virtual void
  process(const std::shared_ptr<Array<float, 2> >& src,
    const std::shared_ptr<Array<float, 2> >      & dst)
  {
    process2D(src, dst);
  }

  bool
  supportsDimensions(size_t dims) const override
  {
    return (dims >= 1 && dims <= 3);
  }

  // --- Array<T, N> overloads with direct casting ---

  void
  process1D(const std::shared_ptr<Array<float, 1> >& src, const std::shared_ptr<Array<float, 1> >& dst) override
  {
    processPointData(
      static_cast<const float *>(src->getRawDataPointer()),
      static_cast<float *>(dst->getRawDataPointer()),
      src->refAs1D().size()
    );
  }

  void
  process2D(const std::shared_ptr<Array<float, 2> >& src, const std::shared_ptr<Array<float, 2> >& dst) override
  {
    processPointData(
      static_cast<const float *>(src->getRawDataPointer()),
      static_cast<float *>(dst->getRawDataPointer()),
      src->refAs1D().size()
    );
  }

  void
  process3D(const std::shared_ptr<Array<float, 3> >& src, const std::shared_ptr<Array<float, 3> >& dst) override
  {
    processPointData(
      static_cast<const float *>(src->getRawDataPointer()),
      static_cast<float *>(dst->getRawDataPointer()),
      src->refAs1D().size()
    );
  }

  // --- std::vector overloads (No casting required) ---

  void
  process1D(const std::vector<float>& src, std::vector<float>& dst)
  {
    if (dst.size() < src.size()) {
      dst.resize(src.size());
    }
    processPointData(src.data(), dst.data(), src.size());
  }

protected:
  virtual void
  processPointData(const float * srcData, float * dstData, size_t totalElements) = 0;
};
}
