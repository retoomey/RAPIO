#include <rArrayFilter.h>
#include <rError.h>

namespace rapio {
/** Percent filter */
class PercentFilter : public ArrayFilter {
public:
  PercentFilter() : ArrayFilter(){ }

  PercentFilter(float percent, int halfX, int halfY, float minFill, std::shared_ptr<ArrayAlgorithm> upstream)
    : ArrayFilter(upstream),
    myPercentile(percent / 100.0f),
    halfX(halfX),
    halfY(halfY)
  {
    // Pre-calculate minimum number of required points
    int totalWindowSize = (2 * halfX + 1) * (2 * halfY + 1);

    myMinFillCount = static_cast<int>(minFill * totalWindowSize + 0.5f);
  }

  /** Introduce to factory */
  static void
  introduceSelf();

  /** Parse string options in the factory */
  virtual bool
  parseOptions(const std::vector<std::string>& parts, std::shared_ptr<ArrayAlgorithm> upstream) override;

  /** Get help for us */
  virtual std::string
  getHelpString() override;

  // Replaces sampleAt and sampleAtIndex with the macro
  DECLARE_FILTER_SAMPLERS

private:

  /** Default to a Median Filter (50th percentile) */
  float myPercentile = 0.5f;

  // Default to an 11x11 spatial window (5 pixels in each direction)

  /** Window size in X is 2*halfX+1 */
  int halfX = 5;

  /** Window size in Y is 2*halfY+1 */
  int halfY = 5;

  int myMinFillCount = 40;
};
}
