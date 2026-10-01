#pragma once
#include <rPointFilter.h>
#include <rConstants.h>
#include <string>
#include <vector>

namespace rapio {
class ThresholdFilter : public PointFilter {
public:
  /** Create a ThresholdFilter */
  ThresholdFilter() = default;

  /** Introduce to factory */
  static void
  introduceSelf();

  /** Get help for us */
  virtual std::string
  getHelpString() override;

  /** Parse string options from the factory */
  virtual bool
  parseOptions(const std::string& params) override;

  /** General threshold on float data */
  virtual void
  processPointData(const float * srcData, float * dstData, size_t totalElements) override;

private:

  /** Min value of threshold, under this is missing */
  float myMin = 0.0f;

  /** Max value of threshold, capping to max */
  float myMax = 100.0f;
};
} // namespace rapio
