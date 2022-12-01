#include "rTerrainBlockage.h"
#include "rRadialSet.h"

// Default terrain blockages
#include "rTerrainBlockageLak.h"
#include "rTerrainBlockage2me.h"

#include <algorithm>

using namespace rapio;

void
TerrainBlockage::introduce(const std::string & key,
  std::shared_ptr<TerrainBlockage>           factory)
{
  Factory<TerrainBlockage>::introduce(key, factory);
}

void
TerrainBlockage::introduceSelf()
{
  // Add Lak's and Toomey's terrain algorithms.
  // You can call introduce(key, another terrain) in your algorithm to add more or replace
  TerrainBlockageLak::introduceSelf();
  TerrainBlockage2me::introduceSelf();
}

std::shared_ptr<TerrainBlockage>
TerrainBlockage::createTerrainBlockage(const std::string & key,
  std::shared_ptr<LatLonGrid>                            aDEM,
  const LLH                                              & radarLocation,
  const LengthKMs                                        & radarRangeKMs,
  const std::string                                      & radarName,
  LengthKMs                                              minTerrainKMs,
  AngleDegs                                              minAngleDegs)
{
  // LogSevere("Factory Terrain Blockage Create Called with key "<<key << "\n");
  auto f = Factory<TerrainBlockage>::get(key);

  if (f == nullptr) {
    LogSevere("Couldn't create TerrainBlockage from key '" << key << "', available: \n");
    auto e = Factory<TerrainBlockage>::getAll();
    for (auto i: e) {
      LogSevere("  '" + i.first + "'\n"); // FIXME: help string later maybe
    }
  } else {
    // Pass onto the factory method
    f = f->create(aDEM, radarLocation, radarRangeKMs, radarName, minTerrainKMs, minAngleDegs);
  }
  return f;
}

TerrainBlockage::TerrainBlockage(std::shared_ptr<LatLonGrid> aDEM,
  const LLH                                                  & radarLocation_in,
  const LengthKMs                                            & radarRangeKMs,
  const std::string                                          & radarName,
  LengthKMs                                                  minTerrainKMs,
  AngleDegs                                                  minAngleDegs)
  : myDEM(aDEM), myRadarLocation(radarLocation_in), myMinTerrainKMs(minTerrainKMs), myMinAngleDegs(minAngleDegs),
  myMaxRangeKMs(radarRangeKMs)
{
  // Make a lookup for our primary data layer, which is the height array
  myDEMLookup = std::make_shared<LatLonGridProjection>(Constants::PrimaryDataName, myDEM.get());
}

//
// Terrain height and height functions.  Feel like they could be cleaner
//
LengthKMs
TerrainBlockage::
getTerrainHeightKMs(const AngleDegs latDegs, const AngleDegs lonDegs)
{
  double aHeightMeters = myDEMLookup->getValueAtLL(latDegs, lonDegs);

  // Still not sure on what this is in our DEM files
  if (aHeightMeters == Constants::MissingData) {
    return 0;
  }
  return aHeightMeters / 1000.0;
}

LengthKMs
TerrainBlockage ::
getHeightKM(const AngleDegs elevDegs,
  const AngleDegs           azDegs,
  const LengthKMs           rnKMs,
  AngleDegs                 & outLatDegs,
  AngleDegs                 & outLonDegs) const
{
  LengthKMs outHeightKMs;

  Project::BeamPath_AzRangeToLatLon(
    myRadarLocation.getLatitudeDeg(),
    myRadarLocation.getLongitudeDeg(),
    azDegs,
    rnKMs,
    elevDegs,

    outHeightKMs,
    outLatDegs,
    outLonDegs
  );

  // FIXME: if this is necessary should be part of the project I think
  if (outLonDegs <= -180) {
    outLonDegs += 360;
  }

  if (outLatDegs > 180) {
    outLatDegs -= 360;
  }

  // Add radar height location to raise beam to correct height
  outHeightKMs += myRadarLocation.getHeightKM();

  return (outHeightKMs);
} // TerrainBlockage::getHeightKM

LengthKMs
TerrainBlockage ::
getHeightAboveTerrainKM(const AngleDegs elevDegs,
  const AngleDegs                       azDegs,
  const LengthKMs                       rnKMs) const
{
  // Get projected height using radar height and angle info
  AngleDegs outLatDegs, outLonDegs;
  LengthKMs outHeightKMs = getHeightKM(elevDegs, azDegs, rnKMs,
      outLatDegs, outLonDegs);

  // If we have terrain, subtract it from the height.  If missing,
  // we assume no change due to terrain.
  double aHeightMeters = myDEMLookup->getValueAtLL(outLatDegs, outLonDegs);

  if (aHeightMeters != Constants::MissingData) {
    outHeightKMs -= (aHeightMeters / 1000.0);
  }
  return (outHeightKMs);
} // TerrainBlockage::getHeightAboveTerrainKM

void
TerrainBlockage::calculateTerrainPerGate(std::shared_ptr<RadialSet> rptr)
{
  // FIXME: check radar name?
  // FIXME: We could make a general RadialSet center gate marcher which could be useful.
  RadialSet& rs = *rptr;

  // We should always have beamwidth array
  auto& beamwidth = rs.getBeamWidthVector()->ref();

  const AngleDegs elevDegs         = rs.getElevationDegs();
  const LengthKMs stationHeightKMs = rs.getLocation().getHeightKM();

  // Create output arrays on the RadialSet.
  // FIXME: API wrap in the RadialSet?  Currently this is accessed by value resolvers
  auto tbcc = rs.addFloat2D(Constants::TerrainCBBPercent, "Dimensionless", { 0, 1 });
  auto& terrainCBBPercent = tbcc->ref();
  auto tpbb = rs.addFloat2D(Constants::TerrainPBBPercent, "Dimensionless", { 0, 1 });
  auto& terrainPBBPercent = tpbb->ref();
  auto tbh = rs.addByte2D(Constants::TerrainBeamBottomHit, "Dimensionless", { 0, 1 });
  auto& terrainBottomHit = tbh->ref();

  // First gate distance
  LengthKMs startKMs = rs.getDistanceToFirstGateM() / 1000.0;
  auto& gwMs         = rs.getGateWidthVector()->ref(); // We 'could' check nulls here
  auto& azDegs       = rs.getAzimuthVector()->ref();
  auto& azSpaceDegs  = rs.getAzimuthSpacingVector()->ref();

  // March over RadialSet
  const size_t numRadials = rs.getNumRadials();
  const size_t numGates   = rs.getNumGates();

  bool hitBottom;

  for (int r = 0; r < numRadials; ++r) {
    const AngleDegs azDeg           = azDegs[r];
    const AngleDegs centerAzDegs    = azDeg + (.5 * azSpaceDegs[r]);
    const LengthKMs gwKMs           = gwMs[r] / 1000.0; // Constant per radial
    const AngleDegs myBeamWidthDegs = beamwidth[r];
    LengthKMs rangeKMs = startKMs;

    float cbb = 0; // Terrain Blockage alg should increase cbb values
    for (int g = 0; g < numGates; ++g) {
      LengthKMs centerRangeKMs = rangeKMs + (.5 * gwKMs);

      terrainPBBPercent[r][g] = 0;
      hitBottom = false;
      calculatePercentBlocked(stationHeightKMs, myBeamWidthDegs,
        elevDegs, centerAzDegs, centerRangeKMs,
        cbb, terrainPBBPercent[r][g], hitBottom);

      terrainBottomHit[r][g]  = hitBottom ? 1 : 0;
      terrainCBBPercent[r][g] = cbb;

      rangeKMs += gwKMs;
    }
  }
} // TerrainBlockage::calculateTerrainPerGate
