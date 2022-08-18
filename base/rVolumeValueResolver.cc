#include "rVolumeValueResolver.h"
#include "rBinaryTable.h"
#include "rRadialSet.h"
#include "rConfigRadarInfo.h"

using namespace rapio;

/** Showing elevation angle of the contributing tilt below us */
class TestResolver1 : public VolumeValueResolver
{
public:
  virtual void
  calc(VolumeValue& vv) override
  {
    if (vv.lower != nullptr) {
      RadialSet& lowerr = *((RadialSet *) vv.lower);
      auto angle        = lowerr.getElevationDegs();
      vv.dataValue = angle;
    } else {
      vv.dataValue = Constants::MissingData;
    }
  }
};

void
VolumeValueResolver::heightForDegreeShift(VolumeValue& vv, DataType * set, AngleDegs delta, LengthKMs& heightKMs)
{
  if (set == nullptr) { return; }

  LengthKMs outRangeKMs; // not needed, we want the height
  RadialSet& r         = *((RadialSet *) set);
  const double radElev = (r.getElevationDegs() + delta) * DEG_TO_RAD; // half beamwidth of 1 degree
  const double elevTan = tan(radElev);                                // maybe we can cache these per point?
  const double elevCos = cos(radElev);

  Project::Cached_BeamPath_LLHtoAttenuationRange(vv.cHeight,
    vv.sinGcdIR, vv.cosGcdIR, elevTan, elevCos, heightKMs, outRangeKMs);
}

// Get value and height in RadialSet at a given range
bool
VolumeValueResolver::valueAndHeight(VolumeValue& vv, DataType * set, LengthKMs& atHeightKMs, float& out)
{
  if (set == nullptr) { return false; }

  bool have    = false;
  RadialSet& r = *((RadialSet *) set);

  // Projection of height range using attentuation
  LengthKMs outRangeKMs;

  Project::Cached_BeamPath_LLHtoAttenuationRange(vv.cHeight,
    vv.sinGcdIR, vv.cosGcdIR, r.getElevationTan(), r.getElevationCos(), atHeightKMs, outRangeKMs);

  // Projection of azimuth, range to data gate/radial value
  int radial, gate;

  if (r.getRadialSetLookupPtr()->getRadialGate(vv.azDegs, outRangeKMs * 1000.0, &radial, &gate)) {
    const auto& data = r.getFloat2DRef();
    out  = data[radial][gate];
    have = true;

    auto tptr = r.getFloat2D("TerrainPercent");
    if (tptr != nullptr) { // FIXME: vv.haveTerrain or something
      // Ok so check terrain.  50% or more blockage we become unavailable and we don't have it...

      // Shouldn't I store percent here not inter..eh don't know.  Maybe int
      const auto& t = r.getFloat2D("TerrainPercent")->ref();
      if (t[radial][gate] > 50) {
        out  = Constants::DataUnavailable; // meaningless I think. Bleh mask calculation
        have = false;
      } else {
        // How to apply the silly thing?
        if (Constants::isGood(out)) {
          out  = out * (1.0f - (float) (t[radial][gate] / 100.0f)); // assume 0 is the default
          have = true;
        }
      }
      // Starting to think the 50 thing not quite right.  Or bleh
      // if (t[radial][gate] > 50){
      //   out = Constants::DataUnavailable; // meaningless I think. Bleh mask calculation
      //   have = false;
      // }
    }
  }

  return have;
} // VolumeValueResolver::valueAndHeight
