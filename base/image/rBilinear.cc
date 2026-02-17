#include <rBilinear.h>
#include <rError.h>
#include <rFactory.h>

using namespace rapio;
using namespace std;

void
Bilinear::introduceSelf()
{
  std::shared_ptr<ArraySampler> newOne = std::make_shared<Bilinear>();
  Factory<ArraySampler>::introduce("bilinear", newOne);
};

bool
Bilinear::parseOptions(const std::vector<std::string>& parts, std::shared_ptr<ArrayAlgorithm> upstream)
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
      fLogSevere("Warning: 'bilinear' expects 2 parameters (w:h). Ignoring extra {} args.",
        parts.size() - 3);
    }
  } catch (const std::exception& e) {
    fLogSevere("Bilinear param error: {}", e.what());
    return false;
  }
  return true;
}

std::string
Bilinear::getHelpString()
{
  return "{width=3}:{height=3} -- Bilinear interpolation.";
}

bool
Bilinear::sampleAt(float inI, float inJ, float& out)
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
    // 1. Geometry: Where is this point relative to the sample? (Don't wrap this!)
    const int spatial_yat = static_cast<int>(inI) + i;

    // 2. Topology: Where is this data in memory? (Wrap/Clamp this!)
    int memory_yat = spatial_yat;

    // If the lat is invalid, all the lons in the row will be invalid
    if (resolveX(memory_yat)) {
      // For the change in lon row...
      for (int j = -halfNJ; j <= halfNJ; ++j) {
        // Geometry vs Topology for X
        const int spatial_xat = static_cast<int>(inJ) + j;
        int memory_xat        = spatial_xat;

        // ...if lon valid, check if a good value
        if (resolveY(memory_xat)) {
          // ACCESS: Use the Resolved Memory Index
          float& val = (*myRefIn)[memory_yat][memory_xat];

          // ...if the data value good, add weight to total...
          if (Constants::isGood(val)) {
            // WEIGHTING: Use the Unwrapped Spatial Index
            // If we used memory_yat here, a wrapped point would be "360 units away"
            const float dx = std::abs(inI - spatial_yat);
            const float dy = std::abs(inJ - spatial_xat);

            const float wt = (1 - dx) * (1 - dy);
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
endbilinear:;
  return true;
} // Bilinear::remap
