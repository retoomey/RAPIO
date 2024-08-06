#include "rVolumeValueResolver.h"
#include "rBinaryTable.h"
#include "rRadialSet.h"
#include "rConfigRadarInfo.h"
#include "rFactory.h"
#include "rColorTerm.h"
#include "rStrings.h"

using namespace rapio;

void
AzimuthVVResolver::calc(VolumeValue * vvp)
{
  // Output value is the virtual azimuth
  auto& vv = *vvp;

  vv.dataValue = vv.virtualAzDegs;
}

void
RangeVVResolver::calc(VolumeValue * vvp)
{
  // Output value is the virtual range
  auto& vv = *vvp;

  vv.dataValue = vv.virtualRangeKMs;
}

void
TerrainVVResolver::calc(VolumeValue * vvp)
{
  // bool haveLower = queryLayer(vv, VolumeValueResolver::lower);
  auto& vv       = *vvp;
  bool haveLower = queryLower(vv);

  // vv.dataValue = vv.lLayer.beamHitBottom ? 1.0: 0.0;
  // vv.dataValue = vv.lLayer.terrainPBBPercent;
  if (vv.getLowerValue().beamHitBottom) {
    // beam bottom on terrain we'll plot as unavailable
    vv.dataValue = Constants::DataUnavailable;
  } else {
    vv.dataValue = vv.getLowerValue().terrainCBBPercent;
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
VolumeValueResolver::introduce(const std::string & key,
  std::shared_ptr<VolumeValueResolver>           factory)
{
  Factory<VolumeValueResolver>::introduce(key, factory);
}

void
VolumeValueResolver::introduceSelf()
{
  // FIXME: no default yet, these are currently in fusion until debugged
  RangeVVResolver::introduceSelf();
  AzimuthVVResolver::introduceSelf();
  TerrainVVResolver::introduceSelf();
}

std::string
VolumeValueResolver::introduceHelp()
{
  std::string help;

  help += "Value Resolver algorithms determine how values/weights are calculated for a grid location.\n";
  return help;
}

void
VolumeValueResolver::introduceSuboptions(const std::string& name, RAPIOOptions& o)
{
  auto e = Factory<VolumeValueResolver>::getAll();

  for (auto i: e) {
    o.addSuboption(name, i.first, i.second->getHelpString(i.first));
  }
  // There's no 'non' volume value resolver
  // o.setEnforcedSuboptions(name, false);
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

  Project::Cached_BeamPath_LLHtoAttenuationRange(vv.getRadarHeightKMs(),
    vv.sinGcdIR, vv.cosGcdIR, elevTan, elevCos, heightKMs, outRangeKMs);
}
