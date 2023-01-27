#include "rVolumeValueResolver.h"
#include "rBinaryTable.h"
#include "rRadialSet.h"
#include "rConfigRadarInfo.h"
#include "rFactory.h"

using namespace rapio;

void
AzimuthVVResolver::calc(VolumeValue& vv)
{
  // Output value is the virtual azimuth
  vv.dataValue = vv.virtualAzDegs;
}

void
RangeVVResolver::calc(VolumeValue& vv)
{
  // Output value is the virtual range
  vv.dataValue = vv.virtualRangeKMs;
}

void
TerrainVVResolver::calc(VolumeValue& vv)
{
  bool haveLower = queryLayer(vv, vv.lower, vv.lLayer);

  // vv.dataValue = vv.lLayer.beamHitBottom ? 1.0: 0.0;
  // vv.dataValue = vv.lLayer.terrainPBBPercent;
  if (vv.lLayer.beamHitBottom) {
    // beam bottom on terrain we'll plot as unavailable
    vv.dataValue = Constants::DataUnavailable;
  } else {
    vv.dataValue = vv.lLayer.terrainCBBPercent;
    // Super small we'll go unavailable...
    if (vv.dataValue < 0.02) {
      vv.dataValue = Constants::MissingData;
    } else {
      // Otherwise scale a bit to show up with colormap better
      // 0 to 10000
      vv.dataValue *= 100;
      vv.dataValue  = vv.dataValue * vv.dataValue;
    }
  }
}

void
VolumeValueResolver::introduceSelf()
{
  // FIXME: no default yet, these are currently in fusion until debugged
  RangeVVResolver::introduceSelf();
  AzimuthVVResolver::introduceSelf();
  TerrainVVResolver::introduceSelf();
}

std::shared_ptr<VolumeValueResolver>
VolumeValueResolver::createVolumeValueResolver(
  const std::string & key,
  const std::string & params)
{
  auto f = Factory<VolumeValueResolver>::get(key);

  if (f == nullptr) {
    LogSevere("Couldn't create VolumeValueResolver from key '" << key << "', available: \n");
    auto e = Factory<VolumeValueResolver>::getAll();
    for (auto i: e) {
      LogSevere("  '" + i.first + "'\n"); // FIXME: help string later maybe
    }
  } else {
    // Pass onto the factory method
    f = f->create(params);
  }
  return f;
}

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
