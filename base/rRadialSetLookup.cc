#include "rRadialSetLookup.h"
#include "rRadialSet.h"

#include <iostream>

using namespace rapio;

void
RadialSetProjection ::
initToRadialSet(RadialSet& rs)
{
  // Get the number of gates
  const size_t num_radials = rs.getNumRadials();

  myNumGates = (num_radials > 0) ? rs.getNumGates() : 0;

  // This should be per radial shouldn't it? In other words,
  // we should find the azimuth index and then use the gatewidth
  // cooresponding to it.  The assumption is that gatewidths are constant
  // for the RadialSet.
  myGateWidthM       = rs.getGateWidthKMs() * 1000.0;
  myDistToFirstGateM = rs.getDistanceToFirstGateM();
  myDistToLastGateM  = myDistToFirstGateM + (myNumGates * myGateWidthM);

  const auto& azimuths   = rs.getFloat1D("Azimuth")->ref();
  const auto& azimuthsSP = rs.getFloat1D("BeamWidth")->ref();
  const int maxSize      = int(myAzToRadialNum.size());
  const int fullCircle   = 360 * myAccuracy;

  myAccuracy360 = fullCircle;

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
} // RadialSetProjection::initToRadialSet

RadialSetProjection::RadialSetProjection(const std::string& layer, RadialSet * owner, int acc)
  : DataProjection(layer),
  myAccuracy(acc),
  myAzToRadialNum(367 * acc + 1, -1)
{
  auto& r = *owner;

  // Initialize to given RadialSet, however
  // accuracy and storage is now fixed.
  initToRadialSet(r);

  // Weak pointer to layer to use.  We only exist as long as RadialSet is valid
  my2DLayer = r.getFloat2D(layer.c_str())->ptr();

  // Cache stuff from RadialSet for speed
  const auto l = r.getLocation();

  myCenterLatDegs = l.getLatitudeDeg();
  myCenterLonDegs = l.getLongitudeDeg();
}

double
RadialSetProjection::getValueAtLL(double latDegs, double lonDegs)
{
  // Translate from Lat Lon to az/range
  AngleDegs azDegs;
  float rangeMeters;
  double value;
  int radial, gate;

  Project::LatLonToAzRange(myCenterLatDegs, myCenterLonDegs, latDegs, lonDegs, azDegs, rangeMeters);
  getValueAtAzRange(azDegs, rangeMeters / 1000.0, value, radial, gate);
  return value;
}

bool
RadialSetProjection::LLCoverageCenterDegree(const float degreeOut, const size_t numRows, const size_t numCols,
  float& topDegs, float& leftDegs, float& deltaLatDegs, float& deltaLonDegs)
{
  Project::createLatLonGrid(myCenterLatDegs, myCenterLonDegs, degreeOut, numRows, numCols, topDegs, leftDegs,
    deltaLatDegs, deltaLonDegs);
  return true;
}

bool
RadialSetProjection::LLCoverageFull(size_t& numRows, size_t& numCols,
  float& topDegs, float& leftDegs, float& deltaLatDegs, float& deltaLonDegs)
{
  // There isn't really a 'full' for radialset...so use some defaults
  numRows = 1000;
  numCols = 1000;
  double degreeOut = 10;

  // Our center location is the standard location
  Project::createLatLonGrid(myCenterLatDegs, myCenterLonDegs, degreeOut, numRows, numCols, topDegs, leftDegs,
    deltaLatDegs, deltaLonDegs);
  return true;
}
