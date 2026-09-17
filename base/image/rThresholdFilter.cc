#include <rThresholdFilter.h>
#include <rFactory.h>
#include <rError.h>
#include <algorithm>

using namespace rapio;

void
ThresholdFilter::introduceSelf()
{
  std::shared_ptr<ArrayFilter> newOne = std::make_shared<ThresholdFilter>();
  Factory<ArrayFilter>::introduce("threshold", newOne);
}

std::string
ThresholdFilter::getHelpString()
{
  return fmt::format("{{minvalue={:g}}}:{{maxvalue={:g}}} -- Threshold (< min missing, > max clamped).",
           myMin, myMax);
}

bool
ThresholdFilter::parseOptions(const std::vector<std::string>& parts)
{
  // getParam<float>(parts, 1, myMin);
  // getParam<float>(parts, 2, myMax);
  try {
    if (parts.size() > 1) { myMin = std::stof(parts[1]); }
    if (parts.size() > 2) { myMax = std::stof(parts[2]); }
    if (myMin > myMax) {
      std::swap(myMin, myMax);
    }
  } catch (const std::exception& e) {
    fLogSevere("ThresholdFilter param error: {}", e.what());
    return false;
  }
  return true;
}

void
ThresholdFilter::process(std::shared_ptr<Array<float, 2> > src,
  std::shared_ptr<Array<float, 2> >                        dst)
{
  if (!src || !dst) { return; }

  // Grab 1D views of the memory
  auto srcData = src->refAs1D();
  auto dstData = dst->refAs1D();

  // Linearly blast through the array in a single pass
  for (size_t i = 0; i < srcData.size(); ++i) {
    float val = srcData[i];

    if (!Constants::isGood(val)) {
      dstData[i] = val; // Keep missing/unavailable flags intact
    } else if (val < myMin) {
      dstData[i] = Constants::MissingData;
    } else if (val > myMax) {
      dstData[i] = myMax; // Clamp to max
    } else {
      dstData[i] = val;
    }
  }
}
