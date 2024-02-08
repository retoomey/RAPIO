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

  isGood = Constants::isGood(value);

  // Max of beamwidth or the tilt below us.  If too large a spread we
  // treat as if it's not there
  // double alphaBottom = layer.beamWidth; 'spokes and jitter' on elevation intersections
  // when using inBeam for mask.  Probably doesn't have effect anymore

  // Branchless version.  Update the alphaBottom based on if spread is reasonable
  const bool spreadReasonable = (spreadDegs > 1.0) && (spreadDegs <= MAX_SPREAD_DEGS);
  const double alphaBottom    = (spreadReasonable * spreadDegs) // True condition: Use full spread
    + (!spreadReasonable * 1.0);                                // False condition: Default to 1.0

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

namespace {
// Inline for clarity.  In theory branchless will be faster than if/then thus our 'always' calculating
inline void
countValue(const bool good, const double wt, const double value, bool& missingMask, double& totalWt, double& totalsum,
  size_t& count)
{
  static const float ELEV_THRESH = .45;    // -E Smoothing of w2merger
  const bool thresh  = (wt > ELEV_THRESH); // Are we in the threshhold?
  const bool doCount = (good && thresh);   // Do we count the value?

  missingMask = missingMask || thresh;  // Missing if in thresh (not used if we have count anyway)
  totalWt    += doCount * (wt);         // Add to total weight if counted
  totalsum   += doCount * (wt * value); // Add to total sum if counted
  count      += doCount;                // Increase value counter
}
}

void
LakResolver1::calc(VolumeValue& vv)
{
  // Count each value if it contributes, if masks are covered than make missing
  double totalWt  = 0.0;
  double totalsum = 0.0;
  size_t count    = 0;

  // ------------------------------------------------------------------------------
  // Query information for above and below the location
  // and reference the values above and below us (note post smoothing filters)
  // This isn't done automatically because not every resolver will need all of it.
  bool haveLower = queryLower(vv);
  bool haveUpper = queryUpper(vv);
  bool haveLLower = query2ndLower(vv);
  bool haveUUpper = query2ndUpper(vv);
  //haveLLower = false; Rings
  //haveUUpper = false;

  // Get all four layers in their glorious CPU intensitivity.  These is
  // gathering info like terrain hit, range, height, etc. into the vv
  // database structure.
  // FIXME: Probably want to query closest two and conditionally query
  // the further ones at some point, which will be faster
  //  bool haveLower, haveUpper, haveLLower, haveUUpper;

  //  queryLayers(vv, haveLower, haveUpper, haveLLower, haveUUpper);

  // ------------------------------------------------------------------------------
  // Analyze and Application stage
  //
  // Do our math on the raw information returned by the query database and calculate
  // values for this location in 3d space

  // ------------------------------------------------------------------------------
  // First two enclosing tilts
  //
  // Actually not sure I like this..why should distance between
  // two tilts determine beam strength, but it's the paper math.
  // There's not much difference between using 1 degree extrapolation
  // vs the spread for close elevations in the VCP
  const AngleDegs spread = (haveLower && haveUpper) ? std::abs(
    vv.getUpperValue().elevation - vv.getLowerValue().elevation) : 0.0;
  bool terrainBlockedLower = false;
  bool terrainBlockedUpper = false;
  bool missingMask         = false;

  if (haveLower) {
    // FIXME: Feels like these tilt blocks are begging for a single function now
    bool isGoodLower = false;
    bool inBeamLower = false;
    double lowerWt   = 0.0;
    double lValue;
    analyzeTilt(vv.getLowerValue(), vv.virtualElevDegs, spread,
      isGoodLower, inBeamLower, terrainBlockedLower, lValue, lowerWt);
    countValue(isGoodLower, lowerWt, lValue, missingMask, totalWt, totalsum, count);
  }

  if (haveUpper) {
    bool isGoodUpper = false;
    bool inBeamUpper = false;
    double upperWt   = 0.0;
    double uValue;
    analyzeTilt(vv.getUpperValue(), vv.virtualElevDegs, spread,
      isGoodUpper, inBeamUpper, terrainBlockedUpper, uValue, upperWt);
    countValue(isGoodUpper, upperWt, uValue, missingMask, totalWt, totalsum, count);
  }

  // We always mask missing between the two main tilts that aren't blocked
  // This fills in more than the beam width..it's full spread fill which I guess looks better?
  missingMask = missingMask || (haveUpper && haveLower && !terrainBlockedUpper && !terrainBlockedLower);

  // ------------------------------------------------------------------------------
  // Extra tilts beyond.  Closer to merger.  I suspect with static cache merger
  // actually uses all tilts in the volume, but the weighting drops off so fast that
  // 3-4 tilts is more than enough for us dynamically.
  // FIXME: we might query these optionally to increase speed
  if (haveLLower) {
    bool isGoodLower2 = false;
    bool inBeamLower2 = false;
    double lowerWt2   = 0.0;
    double lValue2;
    bool terrainBlockedLower2 = false;
    const AngleDegs spread2   =
      haveUpper ? std::abs(vv.getUpperValue().elevation - vv.get2ndLowerValue().elevation) : 0.0;
    analyzeTilt(vv.get2ndLowerValue(), vv.virtualElevDegs, spread2,
      isGoodLower2, inBeamLower2, terrainBlockedLower2, lValue2, lowerWt2);
    countValue(isGoodLower2, lowerWt2, lValue2, missingMask, totalWt, totalsum, count);
  }

  if (haveUUpper) {
    bool isGoodUpper2 = false;
    bool inBeamUpper2 = false;
    double upperWt2   = 0.0;
    double uValue2;
    bool terrainBlockedUpper2 = false;
    const AngleDegs spread3   =
      haveLower ? std::abs(vv.get2ndUpperValue().elevation - vv.getLowerValue().elevation) : 0.0;
    analyzeTilt(vv.get2ndUpperValue(), vv.virtualElevDegs, spread3,
      isGoodUpper2, inBeamUpper2, terrainBlockedUpper, uValue2, upperWt2);
    countValue(isGoodUpper2, upperWt2, uValue2, missingMask, totalWt, totalsum, count);
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

    // It looks like the math is the same if we pass the range downstream or
    // resolve it now. Possibly we can compact the range in files better similar
    // to what merger does.  So we'll make the weight just range in stage 2
    //   vv.dataValue   = rw * totalsum; // num
    //   vv.dataWeight1 = rw * totalWt;  // dem
    const double aV = totalsum / totalWt;
    vv.dataValue   = rw * aV; // Stage2 just makes v = dataValue/dataWeight1
    vv.dataWeight1 = rw;
  } else {
    // Background.
    vv.dataValue   = missingMask ? Constants::MissingData : Constants::DataUnavailable;
    vv.dataWeight1 = 1.0;
  }
} // calc
