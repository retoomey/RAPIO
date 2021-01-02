#include "rRadialSetLookup.h"
#include "rRadialSet.h"

#include <iostream>

using namespace rapio;

RadialSetLookup ::
RadialSetLookup(RadialSet& rs, int acc)
  :
  myAccuracy(acc),
  myAzToRadialNum(367 * myAccuracy + 1, -1)
{
  // Initialize to given RadialSet, however
  // accuracy and storage is now fixed.
  initToRadialSet(rs, false);
}

void
RadialSetLookup ::
initToRadialSet(RadialSet& rs, bool blank)
{
  // Clear out old data when reusing memory
  if (blank) {
    for (auto& i:myAzToRadialNum) {
      i = -1;
    }
  }
  myDistToFirstGateM = rs.getDistanceToFirstGateM();
  const size_t num_radials = rs.getNumRadials();

  // Get the number of gates
  if (num_radials > 0) {
    myNumGates = rs.getNumGates();

    // This should be per radial shouldn't it? In other words,
    // we should find the azimuth index and then use the gatewidth
    // cooresponding to it
    const auto& gatewidths = rs.getFloat1D("GateWidth")->ref();
    myGateWidthM = gatewidths[0];
  } else {
    myNumGates = 0;
  }

  const auto& azimuths   = rs.getFloat1D("Azimuth")->ref();
  const auto& azimuthsSP = rs.getFloat1D("BeamWidth")->ref();
  const int maxSize      = int(myAzToRadialNum.size());
  const int fullCircle   = 360 * myAccuracy;

  // For each radial in the source RadialSet...
  for (size_t i = 0; i < num_radials; ++i) {
    const float az     = azimuths[i];
    const size_t nexti = (i == num_radials - 1) ? 0 : i + 1; // circular wrap next i

    const int minaz = int(myAccuracy * az);                         // current azimuth
    int maxaz       = int(myAccuracy * (azimuthsSP[i] + az) + 0.5); // next azimuth using spacing
    int nextaz      = int(myAccuracy * azimuths[nexti]);            // next azimuth

    // Next azimuth probably crosses the zero, so add 360 basically to make it bigger
    // This is assuming clockwise radar.
    if (nextaz < minaz) {
      nextaz += fullCircle;
    }

    // Gap fill radials if within reasonable distance.  Note this means radials are
    // monotonically ordered, right?  Should we enforce radial ordering on RadialSet loading?
    if ((maxaz > minaz) &&               // Clockwise monotonically increasing radar
      ( nextaz > maxaz) &&               // with a gap to next azimuth
      ( (nextaz - maxaz) < myAccuracy) ) // that's not so big as to be a sector scan
    {
      maxaz = nextaz; // Pin to the next azimuth, avoid gaps
    }

    // Ignoring radials out of bounds
    if ((minaz < 0) || (minaz >= maxSize) ||
      ( maxaz < 0) || ( maxaz >= maxSize) )
    {
      continue;
    }

    // Fill in the 'range' of bins mapping the radial coverage
    if (maxaz > minaz) {
      for (int j = minaz; j < maxaz; ++j) {
        myAzToRadialNum[j] = i;
      }
    } else {
      for (int j = maxaz; j < minaz; ++j) {
        myAzToRadialNum[j] = i;
      }
    }
  }
  // Now do the wraparound radial (the radial around zero degrees)
  for (size_t i = fullCircle; i < myAzToRadialNum.size(); ++i) {
    if (myAzToRadialNum[i] >= 0) {
      myAzToRadialNum[i - fullCircle] = myAzToRadialNum[i];
    }
  }
} // RadialSetLookup::initToRadialSet
