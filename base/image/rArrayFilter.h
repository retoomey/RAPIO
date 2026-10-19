#pragma once
#include <rArray.h>
#include <rArrayStage.h>
#include <rError.h>

namespace rapio {
class ArrayFilter : public ArrayStage {
public:
  virtual
  ~ArrayFilter() = default;

  /** Filters create unique copies for the pipeline to
   * separate state for threading or duplicated filters */
  virtual std::shared_ptr<ArrayFilter>
  create() const { return nullptr; }

  // Original process:
  /** Array filters take action from source to destination */
  virtual void
  process(const std::shared_ptr<Array<float, 2> >& src,
    const std::shared_ptr<Array<float, 2> >      & dst){ };

  // Capability query: Does this filter support 1D, 2D, or 3D?
  virtual bool
  supportsDimensions(size_t dims) const { return false; }

  // Type-safe processing endpoints. Default implementations throw or log errors
  // to catch pipeline routing mistakes.
  virtual void
  process1D(const std::shared_ptr<Array<float, 1> >& src, const std::shared_ptr<Array<float, 1> >& dst)
  {
    fLogSevere("1D processing not supported by this filter.");
  }

  // Convenience for std::vector
  virtual void
  process1D(const std::vector<float>& src, std::vector<float>& dst)
  {
    fLogSevere("1D vector processing not supported by this filter.");
  }

  virtual void
  process2D(const std::shared_ptr<Array<float, 2> >& src, const std::shared_ptr<Array<float, 2> >& dst)
  {
    fLogSevere("2D processing not supported by this filter.");
  }

  virtual void
  process3D(const std::shared_ptr<Array<float, 3> >& src, const std::shared_ptr<Array<float, 3> >& dst)
  {
    fLogSevere("3D processing not supported by this filter.");
  }
};

/** Clonable filters implement a new instance on call, this
 * helps if threading or multiple filters of same type called
 * in the pipeline */
template <typename Derived>
class CloneableFilter : public ArrayFilter {
public:
  std::shared_ptr<ArrayFilter>
  create() const override
  {
    return std::make_shared<Derived>();
  }
};
}
