#include "rLakResolver1.h"

using namespace rapio;

namespace {
/** Analyze a tilt layer for contribution to final.  We do this for each of the tilts
 * around the point so we just put the common code here */
static void inline
processTilt(LayerValue& layer, AngleDegs& at, AngleDegs spreadDegs,
  // OUTPUTS:
  bool& isGood, bool& isMask, bool& inBeam, bool& terrainBlocked, double& outvalue, double& weight,

  // Value counts towards merger
  double& totalWt, double& totalsum, size_t& count, bool& thresh)
{
  static const float ELEV_FACTOR = log(0.005); // Formula constant from paper
  // FIXME: These could be parameters fairly easily and probably will be at some point
  static const float TERRAIN_PERCENT  = .50; // Cutoff terrain cumulative blockage
  static const float MAX_SPREAD_DEGS  = 4;   // Max spread of degrees between tilts
  static const float BEAMWIDTH_THRESH = .50; // Assume half-degree to meet beamwidth test
  static const float ELEV_THRESH      = .45; // -E Smoothing of w2merger

  // ------------------------------------------------------------------------------
  // Discard tilts/values that hit terrain
  if ((layer.terrainCBBPercent > TERRAIN_PERCENT) || (layer.beamHitBottom)) {
    terrainBlocked = true;
    // Note terrain blocked means isMask is false (shouldn't affect mask)
    //  countValue(ELEV_THRESH, false, lowerWt2, lValue2,
    //     OUTS: totalWt, totalsum, count, inThreshLower2);
    //     Ok so inThreshLower = wt > ELEV_THRESH.  But wt isn't set right?
    //     no, wt is 0, so it's not in the thresh.  Lucky.
    //     So we can just inThresh = false, Mask = false;
    //     No need to call countValue now which saves time.
    //     But totalWt, totalSum, count not affects since good = false
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
  // isMask = Constants::isMask(value);
  // Mask will be missing unless data unavailable or range folded in data.
  // Note we don't check inBeam here that depends on total counted beams, so will
  // be done as a group later
  isMask = (value != Constants::DataUnavailable) && (value != Constants::RangeFolded);

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

  thresh = (weight > ELEV_THRESH);         // Are we in the threshhold?
  const bool doCount = (isGood && thresh); // Do we count the value?

  totalWt  += doCount * (weight);         // Add to total weight if counted
  totalsum += doCount * (weight * value); // Add to total sum if counted
  count    += doCount;                    // Increase value counter

  outvalue = value;
} // processTilt
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

#if 0
namespace {
// Inline for clarity.  In theory branchless will be faster than if/then thus our 'always' calculating
inline void
countValue(const float ELEV_THRESH, const bool good, const double wt, const double value,
  double& totalWt, double& totalsum, size_t& count, bool& thresh)
{
  thresh = (wt > ELEV_THRESH);           // Are we in the threshhold?
  const bool doCount = (good && thresh); // Do we count the value?

  totalWt  += doCount * (wt);         // Add to total weight if counted
  totalsum += doCount * (wt * value); // Add to total sum if counted
  count    += doCount;                // Increase value counter
}
}
#endif // if 0

void
LakResolver1::calc(VolumeValue * vvp)
{
  // FIXME: Make a parameter at some point
  static const float ELEV_THRESH = .45; // -E Smoothing of w2merger

  // Count each value if it contributes, if masks are covered than make missing
  auto& vv        = *(VolumeValueWeightAverage *) (vvp);
  double totalWt  = 0.0;
  double totalsum = 0.0;
  size_t count    = 0;

  // ------------------------------------------------------------------------------
  // Query information for above and below the location
  // and reference the values above and below us (note post smoothing filters)
  // This isn't done automatically because not every resolver will need all of it.
  bool haveLower  = queryLower(vv);
  bool haveUpper  = queryUpper(vv);
  bool haveLLower = query2ndLower(vv);
  bool haveUUpper = query2ndUpper(vv);
  // haveLLower = false; Rings
  // haveUUpper = false;

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

  // Maskable counted location will be missing in output
  bool lMask          = false;
  bool uMask          = false;
  bool uuMask         = false;
  bool llMask         = false;
  bool inThreshLower  = false;
  bool inThreshUpper  = false;
  bool inThreshLower2 = false;
  bool inThreshUpper2 = false;

  if (haveLower) {
    bool terrainBlockedLower = false;
    bool isGoodLower         = false;
    bool inBeamLower         = false;
    double lowerWt = 0.0;
    double lValue;
    processTilt(vv.getLowerValue(), vv.virtualElevDegs, spread,
      isGoodLower, lMask, inBeamLower, terrainBlockedLower, lValue, lowerWt,
      totalWt, totalsum, count, inThreshLower);

    // countValue(ELEV_THRESH, isGoodLower, lowerWt, lValue, totalWt, totalsum, count, inThreshLower);
  }

  if (haveUpper) {
    bool terrainBlockedUpper = false;
    bool isGoodUpper         = false;
    bool inBeamUpper         = false;
    double upperWt = 0.0;
    double uValue;
    processTilt(vv.getUpperValue(), vv.virtualElevDegs, spread,
      isGoodUpper, uMask, inBeamUpper, terrainBlockedUpper, uValue, upperWt,
      totalWt, totalsum, count, inThreshLower);
    // countValue(ELEV_THRESH, isGoodUpper, upperWt, uValue, totalWt, totalsum, count, inThreshUpper);
  }

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
    processTilt(vv.get2ndLowerValue(), vv.virtualElevDegs, spread2,
      isGoodLower2, llMask, inBeamLower2, terrainBlockedLower2, lValue2, lowerWt2,
      totalWt, totalsum, count, inThreshLower);
    // countValue(ELEV_THRESH, isGoodLower2, lowerWt2, lValue2, totalWt, totalsum, count, inThreshLower2);
  }

  if (haveUUpper) {
    bool isGoodUpper2 = false;
    bool inBeamUpper2 = false;
    double upperWt2   = 0.0;
    double uValue2;
    bool terrainBlockedUpper2 = false;
    const AngleDegs spread3   =
      haveLower ? std::abs(vv.get2ndUpperValue().elevation - vv.getLowerValue().elevation) : 0.0;
    processTilt(vv.get2ndUpperValue(), vv.virtualElevDegs, spread3,
      isGoodUpper2, uuMask, inBeamUpper2, terrainBlockedUpper2, uValue2, upperWt2,
      totalWt, totalsum, count, inThreshLower);
    // countValue(ELEV_THRESH, isGoodUpper2, upperWt2, uValue2, totalWt, totalsum, count, inThreshUpper2);
  }

  if (count > 0) {
    // Calculate range weight factor here, we can still add it in
    // We're assuming range weight factor is the same for each contributing
    // value, which is different from e1, e2...en elevation weights
    const auto rw = rangeToWeight(vv.virtualRangeKMs, myVarianceWeight);

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
    //   vv.topSum   = rw * totalsum; // num
    //   vv.bottomSum = rw * totalWt;  // dem
    const double aV = totalsum / totalWt;
    vv.dataValue = aV;
    vv.topSum    = myGlobalWeight * (rw * aV); // Stage2 just makes v = topSum/bottomSum
    vv.bottomSum = myGlobalWeight * (rw);
  } else {
    // Mask is only calculated iff no good values count.
    // Mask flags are set iff data is actually a maskable (typically missing) at the tilt sample
    bool missingMask = (inThreshLower && lMask); // Single beam contribution
    missingMask |= (inThreshUpper && uMask);     // Single beam contribution
    missingMask |= (inThreshUpper2 && uuMask);   // Single beam contribution needed?
    missingMask |= (inThreshLower2 && llMask);   // Single beam contribution needed?
    missingMask |= (lMask && uMask);             // Smear between if maskable values
                                                 // Note this is why we delayed the in thresh test
    // Background.
    // FIXME: Considering a background flag in VolumeValue
    // Note: dataValue is direct 2D output, top/bottom for stage2 here
    vv.dataValue = missingMask ? Constants::MissingData : Constants::DataUnavailable;
    vv.topSum    = vv.dataValue; // Humm our stage2 actually checks this for missing value
    vv.bottomSum = 1.0;          // ignored for missing
  }
} // calc
