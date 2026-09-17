#pragma once
#include <rArrayFilter.h>
#include <rConstants.h>
#include <string>
#include <vector>

namespace rapio {
class ThresholdFilter : public ArrayFilter {
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
  parseOptions(const std::vector<std::string>& parts) override;

  /** Apply the filter from src to dst */
  virtual void
  process(std::shared_ptr<Array<float, 2> > src,
    std::shared_ptr<Array<float, 2> >       dst) override;

private:

  /** Min value of threshold, under this is missing */
  float myMin = 0.0f;

  /** Max value of threshold, capping to max */
  float myMax = 100.0f;
};
} // namespace rapio
