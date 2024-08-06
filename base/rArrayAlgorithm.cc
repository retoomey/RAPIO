#include <rArrayAlgorithm.h>

using namespace rapio;
using namespace std;

void
ArrayAlgorithm::remapFromTo(std::shared_ptr<LatLonGrid> in, std::shared_ptr<LatLonGrid> out, size_t width,
  size_t height)
{
  setSource(in->getFloat2D());
  setOutput(out->getFloat2D());

  // Param based, we'll set directly
  myWidth  = width;
  myHeight = height;

  // FIXME: Could we do different directions?  N by N2?  Why not?
  LogInfo("Remapping using matrix size of " << myWidth << " by " << myHeight << "\n");

  // Input coordinates
  const LLH inCorner = in->getTopLeftLocationAt(0, 0); // not centered
  const AngleDegs inNWLatDegs = inCorner.getLatitudeDeg();
  const AngleDegs inNWLonDegs = inCorner.getLongitudeDeg();
  const auto inLatSpacingDegs = in->getLatSpacing();
  const auto inLonSpacingDegs = in->getLonSpacing();

  // Output coordinates (FIXME: We could add a getNWLat() and getNWLon() method)
  // Don't we start at the center location, right?
  const LLH outCorner = out->getTopLeftLocationAt(0, 0); // not centered
  const AngleDegs outNWLatDegs = outCorner.getLatitudeDeg();
  const AngleDegs outNWLonDegs = outCorner.getLongitudeDeg();

  const AngleDegs startLat = outNWLatDegs - (out->getLatSpacing() / 2.0); // move south (lat decreasing)
  const AngleDegs startLon = outNWLonDegs + (out->getLonSpacing() / 2.0); // move east (lon increasing)
  const size_t numY        = out->getNumLats();
  const size_t numX        = out->getNumLons();

  // Cell hits yof and xof
  // Note the cell is allowed to be fractional and out of range,
  // since we're doing a matrix 'some' cells might be in the range
  size_t counter = 0;

  AngleDegs atLat = startLat;

  for (size_t y = 0; y < numY; ++y, atLat -= out->getLatSpacing()) {
    const float yof = (inNWLatDegs - atLat) / inLatSpacingDegs;

    AngleDegs atLon = startLon;
    for (size_t x = 0; x < numX; ++x, atLon += out->getLonSpacing()) {
      const float xof = (atLon - inNWLonDegs ) / inLonSpacingDegs;
      float value;
      if (remap(yof, xof, value)) {
        (*myRefOut)[y][x] = value;
        counter++;
      }
    } // endX
  }   // endY

  if (counter > 0) {
    LogInfo("Sample hit counter is " << counter << "\n");
  }
} // ArrayAlgorithm::remapFromTo

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
