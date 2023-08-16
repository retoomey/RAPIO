#include "rLakResolver1.h"

using namespace rapio;

namespace {
/** Analyze a tilt layer for contribution to final.  We do this for each of the tilts
 * around the point so we just put the common code here */
static void inline
analyzeTilt(LayerValue& layer, AngleDegs& at, AngleDegs spreadDegs,
  // OUTPUTS:
  bool& isGood, bool& inBeam, bool& terrainBlocked, double& outvalue, double& weight)
{
  static const float ELEV_FACTOR = log(0.005); // Formula constant from paper
  // FIXME: These could be parameters fairly easily and probably will be at some point
  static const float TERRAIN_PERCENT  = .50; // Cutoff terrain cumulative blockage
  static const float MAX_SPREAD_DEGS  = 4;   // Max spread of degrees between tilts
  static const float BEAMWIDTH_THRESH = .50; // Assume half-degree to meet beamwidth test

  // ------------------------------------------------------------------------------
  // Discard tilts/values that hit terrain
  if ((layer.terrainCBBPercent > TERRAIN_PERCENT) || (layer.beamHitBottom)) {
    terrainBlocked = true;
    return; // don't waste time on other math, or do we need to?
  }

  // ------------------------------------------------------------------------------
  // Formula (6) on page 10 ----------------------------------------------
  // This formula has a alphaTop and alphaBottom to calculate delta (the weight)
  //
  const double alphaTop = std::abs(at - layer.elevation);

  inBeam = alphaTop <= BEAMWIDTH_THRESH;

  const double& value = layer.value;

  if (Constants::isGood(value)) {
    isGood = true; // Use it if it's a good data value
  }
  // Max of beamwidth or the tilt below us.  If too large a spread we
  // treat as if it's not there
  // double alphaBottom = layer.beamWidth; 'spokes and jitter' on elevation intersections
  // when using inBeam for mask.  Probably doesn't have effect anymore
  double alphaBottom = 1.0;

  // Update the alphaBottom based on if spread is reasonable
  if (spreadDegs > alphaBottom) {
    if (spreadDegs <= MAX_SPREAD_DEGS) { // if in the spread range, use the spread
      alphaBottom = spreadDegs;          // Use full spread
    }
  }

  // Weight based on the beamwidth or the spread range
  const double alpha = alphaTop / alphaBottom;

  weight = exp(alpha * alpha * alpha * ELEV_FACTOR); // always

  outvalue = value;
} // analyzeTilt
}

void
LakResolver1::introduceSelf()
{
  std::shared_ptr<LakResolver1> newOne = std::make_shared<LakResolver1>();
  Factory<VolumeValueResolver>::introduce("lak", newOne);
}

std::string
LakResolver1::getHelpString(const std::string& fkey)
{
  return "Based on w2merger math and paper using S-curve interpolation.";
}

std::shared_ptr<VolumeValueResolver>
LakResolver1::create(const std::string & params)
{
  return std::make_shared<LakResolver1>();
}

void
LakResolver1::calc(VolumeValue& vv)
{
  double v = Constants::DataUnavailable; // Final output data value

  static const float ELEV_THRESH = .45; // -E Smoothing of w2merger

  // ------------------------------------------------------------------------------
  // Query information for above and below the location
  // and reference the values above and below us (note post smoothing filters)
  // This isn't done automatically because not every resolver will need all of it.
  // bool haveLower  = queryLayer(vv, VolumeValueResolver::lower);
  // bool haveUpper  = queryLayer(vv, VolumeValueResolver::upper);
  // bool haveLLower = queryLayer(vv, VolumeValueResolver::lower2);
  // bool haveUUpper = queryLayer(vv, VolumeValueResolver::upper2);

  // Get all four layers in their glorious CPU intensitivity
  // FIXME: Probably want to query closest two and conditionally query
  // the further ones at some point, which will be faster
  bool haveLower, haveUpper, haveLLower, haveUUpper;

  queryLayers(vv, haveLower, haveUpper, haveLLower, haveUUpper);

  // ------------------------------------------------------------------------------
  // Analyze stage
  // Do our math on the raw information returned by the query database and calculate
  // values for this location in 3d space
  //

  AngleDegs spread = 0.0;

  if (haveLower && haveUpper) {
    // Actually not sure I like this..why should distance between
    // two tilts determine beam strength, but it's the paper math.
    // There's not much difference between using 1 degree extrapolation
    // vs the spread for close elevations in the VCP
    spread = std::abs(vv.getUpperValue().elevation - vv.getLowerValue().elevation);
  }

  bool isGoodLower = false;
  bool inBeamLower = false;
  double lowerWt   = 0.0;
  double lValue;
  bool terrainBlockedLower = false;

  if (haveLower) {
    analyzeTilt(vv.getLowerValue(), vv.virtualElevDegs, spread,
      isGoodLower, inBeamLower, terrainBlockedLower, lValue, lowerWt);
  }

  bool isGoodUpper = false;
  bool inBeamUpper = false;
  double upperWt   = 0.0;
  double uValue;
  bool terrainBlockedUpper = false;

  if (haveUpper) {
    analyzeTilt(vv.getUpperValue(), vv.virtualElevDegs, spread,
      isGoodUpper, inBeamUpper, terrainBlockedUpper, uValue, upperWt);
  }

  spread = 0.0;
  if (haveLLower && haveUpper) {
    spread = std::abs(vv.getUpperValue().elevation - vv.get2ndLowerValue().elevation);
  }
  bool isGoodLower2 = false;
  bool inBeamLower2 = false;
  double lowerWt2   = 0.0;
  double lValue2;
  bool terrainBlockedLower2 = false;

  if (haveLLower) {
    analyzeTilt(vv.get2ndLowerValue(), vv.virtualElevDegs, spread,
      isGoodLower2, inBeamLower2, terrainBlockedLower2, lValue2, lowerWt2);
  }

  spread = 0;
  if (haveLower && haveUUpper) {
    spread = std::abs(vv.get2ndUpperValue().elevation - vv.getLowerValue().elevation);
  }
  bool isGoodUpper2 = false;
  bool inBeamUpper2 = false;
  double upperWt2   = 0.0;
  double uValue2;
  bool terrainBlockedUpper2 = false;

  if (haveUUpper) {
    analyzeTilt(vv.get2ndUpperValue(), vv.virtualElevDegs, spread,
      isGoodUpper2, inBeamUpper2, terrainBlockedUpper, uValue2, upperWt2);
  }

  // ------------------------------------------------------------------------------
  // Application stage
  //

  // We always mask missing between the two main tilts that aren't blocked
  if (haveUpper && haveLower) {
    if (!terrainBlockedUpper && !terrainBlockedLower) {
      v = Constants::MissingData;
    }
  }

  // Value stuff.  Needs cleanup.  Loop would be cleaner at some point
  double totalWt  = 0.0;
  double totalsum = 0.0;
  size_t count    = 0;

  if (lowerWt > ELEV_THRESH) {
    if (isGoodLower) {
      totalWt  += lowerWt;
      totalsum += (lowerWt * lValue);
      count++;
    } else {
      // Non-terrain blocked (> 0 weight) in threshold should be missing mask
      v = Constants::MissingData;
    }
  }

  if (lowerWt2 > ELEV_THRESH) {
    if (isGoodLower2) {
      totalWt  += lowerWt2;
      totalsum += (lowerWt2 * lValue2);
      count++;
    } else {
      v = Constants::MissingData;
    }
  }

  if (upperWt > ELEV_THRESH) {
    if (isGoodUpper) {
      totalWt  += upperWt;
      totalsum += (upperWt * uValue);
      count++;
    } else {
      v = Constants::MissingData;
    }
  }

  if (upperWt2 > ELEV_THRESH) {
    if (isGoodUpper2) {
      totalWt  += upperWt2;
      totalsum += (upperWt2 * uValue2);
      count++;
    } else {
      v = Constants::MissingData;
    }
  }

  if (count > 0) {
    // Calculate range weight factor here, we can still add it in
    // We're assuming range weight factor is the same for each contributing
    // value, which is different from e1, e2...en elevation weights
    const auto rw = 1.0 / std::pow(vv.virtualRangeKMs, 2); // inverse square

    // Don't wanna lose weight info
    // v = totalsum / totalWt; // Actual value which loses info..so

    // To weight average multiple radars we need the numerator/denominator
    // in order to keep weight normalization when multiplying with other weights
    // Either we send v1e1+v2e2 and e1+e2 and multiple R in stage 2, or we send
    // R(v1e1+v1e2) and R(e1+e2)
    // This is because to add to another radar:
    // R(v1e1+v1e2) + R2(v3e3+v4e4) is needed for numerator and
    // R(e1+e2) + R2(e3+e4) is needed for denominator. Where the ei are the elevation
    // weights for each contributing angle.
    // In other words, you lose weight normalization ability by dividing in advance
    // Ugggh hard to explain..or did I?  We'll have to draw up the formulas in
    // presentation for this to be understood by most.
    vv.dataValue   = rw * totalsum; // num
    vv.dataWeight1 = rw * totalWt;  // dem
  } else {
    vv.dataValue   = v; // v = missing
    vv.dataWeight1 = 0; // mark missing
  }
} // calc
