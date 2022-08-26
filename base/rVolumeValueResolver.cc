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

bool
VolumeValueResolver::queryLayer(VolumeValue& vv, DataType * set, LayerValue& l)
{
  l.gate           = -1;
  l.radial         = -1;
  l.terrainPercent = 0;
  l.value          = Constants::DataUnavailable;

  if (set == nullptr) { return false; }
  bool have    = false;
  RadialSet& r = *((RadialSet *) set);

  // Projection of height range using attentuation
  Project::Cached_BeamPath_LLHtoAttenuationRange(vv.cHeight,
    vv.sinGcdIR, vv.cosGcdIR, r.getElevationTan(), r.getElevationCos(), l.heightKMs, l.rangeKMs);

  // Projection of azimuth, range to data gate/radial value
  if (r.getRadialSetLookupPtr()->getRadialGate(vv.azDegs, l.rangeKMs * 1000.0, &l.radial, &l.gate)) {
    const auto& data = r.getFloat2DRef();
    l.value = data[l.radial][l.gate];
    have    = true;

    auto tptr = r.getFloat2D("TerrainPercent");
    if (tptr != nullptr) { // FIXME: vv.haveTerrain or something
      // Ok so check terrain.  50% or more blockage we become unavailable and we don't have it...
      const auto& t = r.getFloat2D("TerrainPercent")->ref();
      l.terrainPercent = t[l.radial][l.gate];
    }
  }

  return have;
} // VolumeValueResolver::valueAndHeight
