#pragma once
#include <rArray.h>
#include <rArrayStage.h>

namespace rapio {
class ArrayFilter : public ArrayStage {
public:
  virtual
  ~ArrayFilter() = default;

  /** Array filters take action from source to destination */
  virtual void
  process(std::shared_ptr<Array<float, 2> > src,
    std::shared_ptr<Array<float, 2> >       dst) = 0;
};
}
