#include "rLak2DResolver.h"

// FIXME:
// Lots of duplication with the regular LakResolver.  Keeping separate for moment
// this is attempt to handle the single level 2D output product w2merger hacked in
// by doing a ground projection.  I think we have to do everything in 3D and then
// do a different technique to get the '2D' product that we want to match w2merger
// The original idea is to do everything normal, except the beam cutoff...so the
// relative weight of locatl radar tilts will cancel out to 'merge' values locally.
// We then use the distance weight like normal for passing to stage2.
//
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
Lak2DResolver::introduceSelf()
{
  std::shared_ptr<Lak2DResolver> newOne = std::make_shared<Lak2DResolver>();
  Factory<VolumeValueResolver>::introduce("lak2D", newOne);
}

std::string
Lak2DResolver::getHelpString(const std::string& fkey)
{
  return "Based on w2merger 2D ground projection.";
}

std::shared_ptr<VolumeValueResolver>
Lak2DResolver::create(const std::string & params)
{
  return std::make_shared<Lak2DResolver>();
}

namespace {
// Inline for clarity.  In theory branchless will be faster than if/then thus our 'always' calculating
inline void
countValue(const bool good, const double wt, const double value, bool& missingMask, double& totalWt, double& totalsum,
  size_t& count)
{
  // FIRST Attempt ignoring the threshold.  This will keep relative single radar
  // weights 'merging' together..then in theory pass on to the next stage using
  // only range as a weight.
  // FIXME: Mask and terrain still good or do we tweak?
  // static const float ELEV_THRESH = .45;    // -E Smoothing of w2merger
  // const bool thresh  = (wt > ELEV_THRESH); // Are we in the threshhold?
  const bool thresh  = true;             // We're doing 2D so no 3D cut off here
  const bool doCount = (good && thresh); // Do we count the value?

  missingMask = missingMask || thresh;  // Missing if in thresh (not used if we have count anyway)
  totalWt    += doCount * (wt);         // Add to total weight if counted
  totalsum   += doCount * (wt * value); // Add to total sum if counted
  count      += doCount;                // Increase value counter
}
}

void
Lak2DResolver::calc(VolumeValue * vvp)
{
  // Count each value if it contributes, if masks are covered than make missing
  auto& vv = *(VolumeValueWeightAverage *) (vvp);

  // ------------------------------------------------------------------------------
  // Query information for the location
  // For the 'one' volume type, we have a single upper layer always (if we have data)
  // FIXME: If this works we might make API more obvious.  This is because the
  // search is set up to hit immediately, giving us null null, tilt, null for the layers.
  // Basically our virtual subtype angle is always < max.
  // FIXME: Obviously reduce all the mess in this class if this works.
  if (queryUpper(vv)) {
    const double& value = vv.getUpperValue().value;
    if (Constants::isGood(value)) {
      // This weight is for merging with other radars...so still need it.
      const auto rw   = 1.0 / std::pow(vv.virtualRangeKMs, 2); // inverse square
      const double aV = value;                                 // Only hit value counts, no vertical interpolation/weighting
      vv.dataValue = aV;
      vv.topSum    = rw * aV; // Stage2 just makes v = topSum/bottomSum
      vv.bottomSum = rw;
    } else {
      // Not a good data value, but we hit the product, so missing.
      vv.dataValue = Constants::MissingData;
      vv.topSum    = vv.dataValue;
      vv.bottomSum = 1.0;
    }
  } else {
    // No data, unavailable
    vv.dataValue = Constants::DataUnavailable;
    vv.topSum    = vv.dataValue;
    vv.bottomSum = 1.0;
  }
} // calc
