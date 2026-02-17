#pragma once

#include <rArraySampler.h>

namespace rapio {
/**
 * @brief Perform nearest neighbor sampling on a 2D grid.
 * @ingroup rapio_image
 **/
class NearestNeighbor : public ArraySampler {
public:

  /** Create nearest neighbor sampler */
  NearestNeighbor() : ArraySampler()
  { }

  /** Introduce to factory */
  static void
  introduceSelf();

  // No parse options since we're identity pretty much

  /** Get help for us */
  virtual std::string
  getHelpString() override;

  /** Get value in source at index location */
  virtual bool
  sampleAt(float inI, float inJ, float& out) override;
};
}
