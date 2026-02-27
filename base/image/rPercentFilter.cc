#include <rPercentFilter.h>
#include <rFactory.h>
#include <rError.h>

using namespace rapio;

void
PercentFilter::introduceSelf()
{
  std::shared_ptr<ArrayFilter> newOneo = std::make_shared<PercentFilter>();
  Factory<ArrayFilter>::introduce("percent", newOneo);
};

bool
PercentFilter::parseOptions(const std::vector<std::string>& parts,
  std::shared_ptr<ArrayAlgorithm>                         upstream)
{
  // We need temporary variables for things that require math before assignment
  float cutoff  = myPercentile * 100.0f; // Default from header
  float minFill = 0.33f;

  getParam(parts, 1, cutoff);
  getParam(parts, 2, halfX);
  getParam(parts, 3, minFill);

  // Derived default: halfY defaults to whatever halfX is NOW.
  halfY = halfX;
  getParam(parts, 4, halfY); // Overwrite only if the user specifically provided it

  // Apply math to final state
  myPercentile   = cutoff / 100.0f;
  myMinFillCount = static_cast<int>(minFill * (2 * halfX + 1) * (2 * halfY + 1) + 0.5f);

  return true;
}

std::string
PercentFilter::getHelpString()
{
  return "{percent=50}:{halfSizeX=5}:{minFill=0.33}:{halfSizeY=5} -- Nth percentile filter.";
}

template <typename T>
bool
PercentFilter::doSample(T x, T y, float& out)
{
  std::vector<float> neighbors;

  // Pre-allocate to prevent memory fragmentation in tight sliding window loops
  neighbors.reserve((2 * halfX + 1) * (2 * halfY + 1));

  // Iterate through the neighborhood
  for (int m = -halfX; m <= halfX; ++m) {
    for (int n = -halfY; n <= halfY; ++n) {
      float val;

      // MAGIC HAPPENS HERE:
      // If T is int, (x + m) resolves to int, calling callUpstream(int, int)
      // If T is float, (x + m) resolves to float, calling callUpstream(float, float)
      if (callUpstream(x + m, y + n, val)) {
        if (val != Constants::MissingData) {
          neighbors.push_back(val);
        }
      }
    }
  }

  // Check if we have enough data to satisfy the fill requirement
  if (neighbors.size() > static_cast<size_t>(myMinFillCount)) {
    size_t nIndex = static_cast<size_t>(neighbors.size() * myPercentile);
    if (nIndex >= neighbors.size()) { nIndex = neighbors.size() - 1; }

    // Partially sort to find the Nth value (O(N) operation instead of O(N log N))
    std::nth_element(neighbors.begin(), neighbors.begin() + nIndex, neighbors.end());
    out = neighbors[nIndex];
    return true;
  }

  out = Constants::MissingData;
  return true;
} // PercentFilter::doSample

// Instantiate the virtual overrides using the template above
DEFINE_FILTER_SAMPLERS(PercentFilter)
