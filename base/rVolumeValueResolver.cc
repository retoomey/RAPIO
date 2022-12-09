#include "rVolumeValueResolver.h"
#include "rBinaryTable.h"
#include "rRadialSet.h"
#include "rConfigRadarInfo.h"

using namespace rapio;

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

bool
VolumeValueResolver::queryLayer(VolumeValue& vv, DataType * set, LayerValue& l)
{
  l.clear();

  if (set == nullptr) { return false; }
  bool have    = false;
  RadialSet& r = *((RadialSet *) set);

  l.elevation = r.getElevationDegs();

  // Projection of height range using attentuation
  Project::Cached_BeamPath_LLHtoAttenuationRange(vv.cHeight,
    vv.sinGcdIR, vv.cosGcdIR, r.getElevationTan(), r.getElevationCos(), l.heightKMs, l.rangeKMs);

  // Projection of azimuth, range to data gate/radial value
  if (r.getRadialSetLookupPtr()->getRadialGate(vv.virtualAzDegs, l.rangeKMs * 1000.0, &l.radial, &l.gate)) {
    const auto& data = r.getFloat2DRef();
    l.value = data[l.radial][l.gate];
    have    = true;

    // Get the beamwidth for the radial
    const auto& bw = r.getBeamWidthVector()->ref();
    l.beamWidth = bw[l.radial];

    // Check for Terrain information arrays
    auto hitptr = r.getByte2D(Constants::TerrainBeamBottomHit);
    auto pbbptr = r.getFloat2D(Constants::TerrainPBBPercent);
    auto cbbptr = r.getFloat2D(Constants::TerrainCBBPercent);
    if ((hitptr != nullptr) && (pbbptr != nullptr) && (cbbptr != nullptr)) {
      l.haveTerrain = true;
      const auto& cbb = cbbptr->ref();
      const auto& pbb = pbbptr->ref();
      const auto& hit = hitptr->ref();
      l.terrainCBBPercent = cbb[l.radial][l.gate];
      l.terrainPBBPercent = pbb[l.radial][l.gate];
      l.beamHitBottom     = (hit[l.radial][l.gate] != 0);
    }
  } else {
    // Outside of coverage area we'll remove virtual values
    l.gate   = -1;
    l.radial = -1;
  }

  return have;
} // VolumeValueResolver::valueAndHeight
