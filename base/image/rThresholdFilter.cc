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
  if (parts.size() < 3) {
    // Use defaults if not enough params
    return true;
  }

  try {
    // Handle our params
    myMin = std::stoul(parts[1]);
    myMax = std::stoul(parts[2]);
    if (myMin > myMax) { }

    if (parts.size() > 3) {
      fLogSevere("Warning: 'Threshold' expects 2 parameters (min:max). Ignoring extra {} args.",
        parts.size() - 3);
    }
  } catch (const std::exception& e) {
    fLogSevere("Threshold param error: {}", e.what());
    return false;
  }
  return true;
}

std::string
ThresholdFilter::getHelpString()
{
  return "{minvalue=0}:{maxvalue=100} -- Threshold (< min missing, > max clamped).";
}

bool
ThresholdFilter::sampleAt(float u, float v, float& out)
{
  float val;

  if (myUpstream->sampleAt(u, v, val)) {
    const float result = (val > myMax) ? myMax :
      (val < myMin) ? Constants::MissingData : val;
    out = result;
    return true;
  }
  return false;
} // ThresholdFilter::remap
