#include <rArrayFilter.h>
#include <rError.h>

namespace rapio {
/** A simple thresholding filter */
class ThresholdFilter : public ArrayFilter {
public:
  ThresholdFilter() : ArrayFilter(){ }

  ThresholdFilter(float min, float max, std::shared_ptr<ArrayAlgorithm> upstream)
    : ArrayFilter(upstream), myMin(min), myMax(max){ }

  /** Introduce to factory */
  static void
  introduceSelf();

  /** Parse string options in the factory */
  virtual bool
  parseOptions(const std::vector<std::string>& parts, std::shared_ptr<ArrayAlgorithm> upstream) override;

  /** Get help for us */
  virtual std::string
  getHelpString() override;

  // Declares sampleAt, sampleAtIndex, and doSample
  DECLARE_FILTER_SAMPLERS

private:

  /** Min value of threshold, under this is missing */
  float myMin = 0.0f;

  /** Max value of threshold, capping to max */
  float myMax = 100.0f;
};
}
