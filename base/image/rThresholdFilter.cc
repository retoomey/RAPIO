#include <rThresholdFilter.h>
#include <rFactory.h>
#include <rError.h>

using namespace rapio;
using namespace std;

void
ThresholdFilter::introduceSelf()
{
  std::shared_ptr<ArrayFilter> newOneo = std::make_shared<ThresholdFilter>();
  Factory<ArrayFilter>::introduce("threshold", newOneo);
};


/** Parse string options in the factory */
bool
ThresholdFilter::parseOptions(const std::vector<std::string>& parts,
  std::shared_ptr<ArrayAlgorithm>                           upstream)
{
  getParam<float>(parts, 1, myMin);
  getParam<float>(parts, 2, myMax);

  if (myMin > myMax) {
    std::swap(myMin, myMax);
  }

  return true;
}

std::string
ThresholdFilter::getHelpString()
{
  return fmt::format("{{minvalue={:g}}}:{{maxvalue={:g}}} -- Threshold (< min missing, > max clamped).",
           myMin, myMax);
}

template <typename T>
bool
ThresholdFilter::doSample(T x, T y, float& out)
{
  float val;

  if (callUpstream(x, y, val)) {
    // The threshold filter math
    out = (val > myMax) ? myMax : (val < myMin) ? Constants::MissingData : val;

    return true;
  }
  return false;
}

DEFINE_FILTER_SAMPLERS(ThresholdFilter)
