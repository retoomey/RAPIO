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

  /** Get value in source at virtual index location */
  bool
  sampleAt(float u, float v, float& out) override;

private:

  /** Min value of threshold, under this is missing */
  float myMin;

  /** Max value of threshold, capping to max */
  float myMax;
};
}
