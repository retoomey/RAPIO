#include "rVelResolver.h"

using namespace rapio;

void
VelResolver::introduceSelf()
{
  std::shared_ptr<VelResolver> newOne = std::make_shared<VelResolver>();
  Factory<VolumeValueResolver>::introduce("vel", newOne);
}

std::string
VelResolver::getHelpString(const std::string& fkey)
{
  return "Experimental velocity gatherer.";
}

std::shared_ptr<VolumeValueResolver>
VelResolver::create(const std::string & params)
{
  return std::make_shared<VelResolver>();
}

namespace {
/** Analyze a tilt layer for contribution to final.  We do this for each of the tilts
 * around the point so we just put the common code here */
static void inline
analyzeTilt(VolumeValueVelGatherer& vv, LayerValue& layer, AngleDegs& at,
  // OUTPUTS:
  float&value, bool& isGood, bool& inBeam, bool& terrainBlocked)
{
  static const float TERRAIN_PERCENT  = .50; // Cutoff terrain cumulative blockage
  static const float BEAMWIDTH_THRESH = .50; // Assume half-degree to meet beamwidth test

  // ------------------------------------------------------------------------------
  // Discard tilts/values that hit terrain
  // I'm assuming we want terrain filtering for velocity, comment out if not
  // wanted
  if ((layer.terrainCBBPercent > TERRAIN_PERCENT) || (layer.beamHitBottom)) {
    terrainBlocked = true;
  }

  // Get beam width distance and threshhold
  const double centerBeamDelta = std::abs(at - layer.elevation);

  inBeam = centerBeamDelta <= BEAMWIDTH_THRESH;

  // Is it a good value and we set it as well
  value  = layer.value;
  isGood = Constants::isGood(value);


  // ------------------------------------------------------------
  // A simple grid value of the velocity at that gate/range
  // just to see CAPPIs and that we're on the right track...
  if (!terrainBlocked && inBeam && isGood) {
    // For the CAPPI debug output of stage1.
    // A simple grid value of the velocity at that gate/range
    vv.dataValue = value;
    // ------------------------------------------------------------
    // Now do the math for stuff for the location
    // We're going to try doing a cressmen in the 2D of the radial sets.
    // Fun fun fun.
    // FIXME: Should be using Constants probably.  This is alpha POC
    const float RAD     = 0.01745329251f;
    const double earthR = 6370949.0;
  }
} // analyzeTilt
}

void
VelResolver::calc(VolumeValue * vvp)
{
  static const float HEIGHT_THRESH_KMS = .5; // Distance cut off

  // The database of input/output values for this grid cell
  auto& vv = *(VolumeValueVelGatherer *) (vvp);

  // ------------------------------------------------------------
  // Query for tilts directly above/below us
  bool haveLower = queryLower(vv);
  bool haveUpper = queryUpper(vv);

  // ------------------------------------------------------------
  // Choose the closest tilt.
  //
  if (haveLower && haveUpper) {
    // We'll do closest by straight up/down kilometers.
    // FIXME: util API for this
    const float dLowerKMs = vv.getAtHeightKMs() - vv.getLowerValue().heightKMs;
    const float dUpperKMs = vv.getUpperValue().heightKMs - vv.getAtHeightKMs();
    float dKMs;
    if (dLowerKMs < dUpperKMs) {
      haveUpper = false;
      dKMs      = dLowerKMs;
    } else {
      haveLower = false;
      dKMs      = dUpperKMs;
    }

    // FIXME: Some max distance toss out the other as well, right?
    if (dKMs > HEIGHT_THRESH_KMS) {
      haveUpper = haveLower = false;
    }
  }

  // ------------------------------------------------------------
  // At this point we have 1 or 0 tilts, so analyze that tilt
  // and then create a single sample grid value
  bool terrainBlocked = false;
  bool inBeam         = false;
  bool isGood         = false;
  float value;

  if (haveLower) {
    analyzeTilt(vv, vv.getLowerValue(), vv.virtualElevDegs,
      value, isGood, inBeam, terrainBlocked);
  } else if (haveUpper) {
    analyzeTilt(vv, vv.getUpperValue(), vv.virtualElevDegs,
      value, isGood, inBeam, terrainBlocked);
  }

  // ------------------------------------------------------------
  // A simple grid value of the velocity at that gate/range
  // just to see CAPPIs and that we're on the right track...
  if (!terrainBlocked && inBeam && isGood) {
    // For the CAPPI debug output of stage1.
    // A simple grid value of the velocity at that gate/range
    // FIXME: Cressmen 2D of the RadialSet most likely.
    vv.dataValue = value;

    // ------------------------------------------------------------
    // Now do the math for stuff for the location
    // We're going to try doing a cressmen in the 2D of the radial sets.
    // Fun fun fun.
    // FIXME: Should be using Constants probably.  This is alpha POC
    // Also we could probably just add methods for Radians
    // FIXME: Just doing calculations for moment.  We'll need work on
    // our stage2 VolumeValueIO object of course to actually output
    const float RAD     = 0.01745329251f;
    const double earthR = 6370949.0;

    const float rLatRad    = RAD * vv.getRadarLatitudeDegs();
    const float rLonRad    = RAD * vv.getRadarLongitudeDegs();
    const LengthKMs rHtKMs = vv.getRadarHeightKMs();

    const float oLatRad    = RAD * vv.getAtLatitudeDegs();
    const float oLonRad    = RAD * vv.getAtLongitudeDegs();
    const LengthKMs oHtKMs = vv.getAtHeightKMs();

    const float x     = fabs(oLonRad - rLonRad) * earthR; // Arc length in meters
    const float y     = fabs(oLatRad - rLatRad) * earthR; // Arc length in meters
    const float z     = fabs(oHtKMs - rHtKMs) * 1000.0;   // Meters
    const float range = sqrtf(x * x + y * y + z * z);     // Distance formula
    const float ux    = x / range;                        // units 'should' cancel so technically we're dimensionless
    const float uy    = y / range;
    const float uz    = z / range;
  } else {
    // My guess for a reasonable missing for this first start
    // Note missing only needed for 2D/3D CAPPI debugging output, we'll be making
    // tables for our stage2.
    if (inBeam) {
      vv.dataValue = Constants::MissingData;
    } else {
      vv.dataValue = Constants::DataUnavailable;
    }
  }
} // calc
