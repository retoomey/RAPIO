#include <rArrayAlgorithm.h>

using namespace rapio;
using namespace std;

bool
NearestNeighbor::remap(float inI, float inJ, float& out)
{
  // for nearest, round to closest cell hit
  const int i = std::round(inI);

  if ((i < 0) || (i >= myMaxI)) {
    return false;
  }
  const int j = std::round(inJ);

  if ((j < 0) || (j >= myMaxJ)) {
    return false;
  }
  out = (*myRefIn)[i][j];
  return true;
}

bool
Cressman::remap(float inI, float inJ, float& out)
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
    const int yat = static_cast<int>(inI) + i;

    // If the lat is invalid, all the lons in the row will be invalid
    if (!((yat < 0) || (yat >= myMaxI))) {
      // For the change in lon row...
      for (int j = -halfNJ; j <= halfNJ; ++j) {
        int xat = static_cast<int>(inJ) + j;

        // ...if lon valid, check if a good value
        if (!((xat < 0) || (xat >= myMaxJ))) {
          float& val = (*myRefIn)[yat][xat];

          // ...if the data value good, add weight to total...
          if (Constants::isGood(val)) {
            const float jDist = (inJ - xat) * (inJ - xat);
            const float iDist = (inI - yat) * (inI - yat);
            const float dist  = std::sqrt(iDist + jDist);

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

bool
Bilinear::remap(float inI, float inJ, float& out)
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
    const int yat = static_cast<int>(inI) + i;

    // If the lat is invalid, all the lons in the row will be invalid
    if (!((yat < 0) || (yat >= myMaxI))) {
      // For the change in lon row...
      for (int j = -halfNJ; j <= halfNJ; ++j) {
        int xat = static_cast<int>(inJ) + j;

        // ...if lon valid, check if a good value
        if (!((xat < 0) || (xat >= myMaxJ))) {
          float& val = (*myRefIn)[yat][xat];

          // ...if the data value good, add weight to total...
          if (Constants::isGood(val)) {
            const float dx = std::abs(inI - yat);
            const float dy = std::abs(inJ - xat);
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
