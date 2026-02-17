#include <rCressman.h>
#include <rFactory.h>
#include <rError.h>

using namespace rapio;
using namespace std;

void
Cressman::introduceSelf()
{
  std::shared_ptr<ArraySampler> newOne = std::make_shared<Cressman>();
  Factory<ArraySampler>::introduce("cressman", newOne);
};

bool
Cressman::parseOptions(const std::vector<std::string>& parts, std::shared_ptr<ArrayAlgorithm> upstream)
{
  if (parts.size() < 3) {
    // Use defaults if not enough params
    return true;
  }

  try {
    // Handle our params
    myWidth  = std::stoul(parts[1]);
    myHeight = std::stoul(parts[2]);

    if (parts.size() > 3) {
      fLogSevere("Warning: 'cressman' expects 2 parameters (w:h). Ignoring extra {} args.",
        parts.size() - 3);
    }
  } catch (const std::exception& e) {
    fLogSevere("Cressman param error: {}", e.what());
    return false;
  }
  return true;
}

bool
Cressman::sampleAt(float inI, float inJ, float& out)
{
  // Iterator over half size to center sample
  // We could store these probably
  const int halfNI = myWidth / 2;
  const int halfNJ = myHeight / 2;

  // Sum up all the weights for each sample
  float tot_wt      = 0;
  float tot_val     = 0;
  float currentMask = Constants::DataUnavailable;
  int n = 0;

  for (int i = -halfNI; i <= halfNI; ++i) {
    // 1. Geometry (Unwrapped): Where is the point spatially?
    const int spatial_yat = static_cast<int>(inI) + i;

    // 2. Topology (Wrapped): Where is the data in memory?
    int memory_yat = spatial_yat;

    // Resolve Boundary (modifies memory_yat if wrapping)
    // If the lat is invalid, all the lons in the row will be invalid
    if (resolveX(memory_yat)) {
      // For the change in lon row...
      for (int j = -halfNJ; j <= halfNJ; ++j) {
        // Geometry vs Topology for J dimension
        const int spatial_xat = static_cast<int>(inJ) + j;
        int memory_xat        = spatial_xat;

        // ...if lon valid, check if a good value
        // Resolve Boundary
        if (resolveY(memory_xat)) {
          float& val = (*myRefIn)[memory_yat][memory_xat];

          // ...if the data value good, add weight to total...
          if (Constants::isGood(val)) {
            // DISTANCE: Use unwrapped spatial index
            // If we used memory_xat here, wrapping would break the distance logic
            const float jDiff = (inJ - spatial_xat);
            const float iDiff = (inI - spatial_yat);

            const float jDist = jDiff * jDiff;
            const float iDist = iDiff * iDiff;

            const float dist = std::sqrt(iDist + jDist);

            // If the distance is extremely small, use the cell exact value
            // to avoid division by zero
            // This also passes on mask when close to a true cell location
            if (dist < std::numeric_limits<float>::epsilon()) {
              out = val;
              goto endcressman;
            }

            float wt = 1.0 / dist;
            tot_wt  += wt;
            tot_val += wt * val;
            ++n;
          } else {
            // If any value in our matrix sampling is missing, we'll use that as a mask
            // if there are no good values to interpolate.  Should work
            if (val == Constants::MissingData) {
              currentMask = Constants::MissingData;
            }
          }
        } // Valid lon
      }   // End lon row
    } // Valid lat
  }   // End lat column

  if (n > 0) {
    out = tot_val / tot_wt;
  } else {
    out = currentMask;
  }
endcressman:;
  return true;
} // Cressman::remap

std::string
Cressman::getHelpString()
{
  return "{width=3}:{height=3} -- Cressman interpolation.";
}
