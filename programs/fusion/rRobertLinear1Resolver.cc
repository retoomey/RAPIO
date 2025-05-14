#include "rRobertLinear1Resolver.h"

using namespace rapio;

void
RobertLinear1Resolver::introduceSelf()
{
  std::shared_ptr<RobertLinear1Resolver> newOne = std::make_shared<RobertLinear1Resolver>();
  Factory<VolumeValueResolver>::introduce("robert", newOne);
}

std::string
RobertLinear1Resolver::getHelpString(const std::string& fkey)
{
  return "Silly simple linear interpolation between tilts.";
}

std::shared_ptr<VolumeValueResolver>
RobertLinear1Resolver::create(const std::string & params)
{
  return std::make_shared<RobertLinear1Resolver>();
}

void
RobertLinear1Resolver::calc(VolumeValue * vvp)
{
  // ------------------------------------------------------------------------------
  // Query information for above and below the location
  auto& vv       = *(VolumeValueWeightAverage *) (vvp);
  bool haveLower = queryLower(vv);
  bool haveUpper = queryUpper(vv);

  // ------------------------------------------------------------------------------
  // In beamwidth calculation
  //
  // FIXME: might be able to optimize by not always calculating in beam width,
  // but it's making my head hurt so just do it always for now
  // The issue is between tilts lots of tan, etc. we might skip

  // Get height of half beamwidth higher
  LengthKMs lowerHeightKMs;
  bool inLowerBeamwidth = false;

  if (haveLower) { // Do we hit the valid gates of the lower tilt?
    heightForDegreeShift(vv, vv.getLower(), vv.getLowerValue().beamWidth / 2.0, lowerHeightKMs);
    inLowerBeamwidth = (vv.getAtHeightKMs() <= lowerHeightKMs);
  }

  // Get height of half beamwidth lower
  LengthKMs upperHeightKMs;
  bool inUpperBeamwidth = false;

  if (haveUpper) { // Do we hit the valid gates of the upper tilt?
    heightForDegreeShift(vv, vv.getUpper(), -(vv.getUpperValue().beamWidth / 2.0), upperHeightKMs);
    inUpperBeamwidth = (vv.getAtHeightKMs() >= upperHeightKMs);
  }

  // ------------------------------------------------------------------------------
  // Background MASK calculation
  //
  // Keep background mask logic separate for now, though would probably be faster
  // doing it in the value calculation.  I recommend doing it this way because
  // logically it frys your brain a bit less.  You can 'have' an upper tilt,
  // but have bad values or missing, etc..so the cases are not one to one
  double v = myUnavailable;

  if (haveUpper && haveLower) { // 11 Four binary possibilities
    v = myMissing;
  } else {
    if (haveUpper) { // 10
      if (inUpperBeamwidth) {
        v = myMissing;
      }
    } else if (haveLower) { // 01
      if (inLowerBeamwidth) {
        v = myMissing;
      }
    } else {
      // v = myUnavailable; // 00
    }
  }
  // Make values unavailable if cumulative blockage is over 50%
  // FIXME: Could be configurable
  if (vv.getUpperValue().terrainCBBPercent > .50) {
    vv.getUpperValue().value = myUnavailable;
  }
  if (vv.getLowerValue().terrainCBBPercent > .50) {
    vv.getLowerValue().value = myUnavailable;
  }
  // Make values unavailable if elevation layer bottom hits terrain
  if (vv.getLowerValue().beamHitBottom) {
    vv.getLowerValue().value = myUnavailable;
  }
  if (vv.getUpperValue().beamHitBottom) {
    vv.getUpperValue().value = myUnavailable;
  }


  // ------------------------------------------------------------------------------
  // Interpolation and application of upper and lower data values
  //
  const double& lValue = vv.getLowerValue().value;
  const double& uValue = vv.getUpperValue().value;

  if (Constants::isGood(lValue) && Constants::isGood(uValue)) {
    // Linear interpolate using heights.  With two values we can do interpolation
    // between the values always, either linear or exponential
    double wt = (vv.getAtHeightKMs() - vv.getLowerValue().heightKMs) / (upperHeightKMs - vv.getUpperValue().heightKMs);
    if (wt < 0) { wt = 0; } else if (wt > 1) { wt = 1; }
    const double nwt = (1.0 - wt);

    const double lTerrain = lValue * (1 - vv.getLowerValue().terrainCBBPercent);
    const double uTerrain = uValue * (1 - vv.getUpperValue().terrainCBBPercent);

    // v = (0.5 + nwt * lValue + wt * uValue);
    v = (0.5 + nwt * lTerrain + wt * uTerrain);
  } else if (inLowerBeamwidth) {
    if (Constants::isGood(lValue)) {
      const double lTerrain = lValue * (1 - vv.getLowerValue().terrainCBBPercent);
      v = lTerrain;
    } else {
      v = lValue; // Use the gate value even if missing, RF, unavailable, etc.
    }
  } else if (inUpperBeamwidth) {
    if (Constants::isGood(uValue)) {
      const double uTerrain = uValue * (1 - vv.getUpperValue().terrainCBBPercent);
      v = uTerrain;
    } else {
      v = uValue;
    }
  }

  // FIXME: haven't tested this since we use Lak's design almost exclusively now
  vv.dataValue = v;
  vv.topSum    = v * vv.virtualRangeKMs;
  vv.bottomSum = vv.virtualRangeKMs;
} // calc
