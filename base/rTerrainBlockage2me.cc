#include "rTerrainBlockage2me.h"
#include "rIODataType.h"

using namespace rapio;

std::shared_ptr<TerrainBlockage>
TerrainBlockage2me::create(const std::string& params,
  const LLH                                 & radarLocation,
  const LengthKMs                           & radarRangeKMs, // Range after this is zero blockage
  const std::string                         & radarName,
  LengthKMs                                 minTerrainKMs,
  AngleDegs                                 minAngleDegs)
{
  // Try to read DEM from provided info
  std::string myTerrainPath = params;

  if (!myTerrainPath.empty()) {
    myTerrainPath += "/";
  }
  std::string file = myTerrainPath + radarName + ".nc";
  std::shared_ptr<LatLonGrid> aDEM = IODataType::read<LatLonGrid>(file);

  return std::make_shared<TerrainBlockage2me>(TerrainBlockage2me(aDEM, radarLocation, radarRangeKMs, radarName,
           minTerrainKMs, minAngleDegs));
}

TerrainBlockage2me::TerrainBlockage2me(std::shared_ptr<LatLonGrid> aDEM,
  const LLH                                                        & radarLocation_in,
  const LengthKMs                                                  & radarRangeKMs,
  const std::string                                                & radarName,
  const LengthKMs                                                  minTerrainKMs,
  const AngleDegs                                                  minAngleDegs)
  : TerrainBlockage(aDEM, radarLocation_in, radarRangeKMs, radarName, minTerrainKMs, minAngleDegs)
{ }

void
TerrainBlockage2me::introduceSelf()
{
  std::shared_ptr<TerrainBlockage2me> newOne = std::make_shared<TerrainBlockage2me>();
  Factory<TerrainBlockage>::introduce("2me", newOne);
}

std::string
TerrainBlockage2me::getHelpString(const std::string& fkey)
{
  return "Polar terrain based on Bech Et Al 2003.";
}

void
TerrainBlockage2me::calculatePercentBlocked(
  // Constants in 3D space information (could be instance)
  LengthKMs stationHeightKMs, AngleDegs beamWidthDegs,
  // Variables in 3D space information.  Should we do center automatically?
  AngleDegs elevDegs, AngleDegs centerAzDegs, LengthKMs centerRangeKMs,
  // Cumulative beam blockage
  float& cbb,
  // Partial beam blockage
  float& pbb,
  // Bottom beam hit
  bool& hit)
{
  AngleDegs outLatDegs, outLonDegs;
  AngleDegs topDegs    = elevDegs + 0.5 * beamWidthDegs;
  AngleDegs bottomDegs = elevDegs - 0.5 * beamWidthDegs;

  // We get back pretty the exact same height doing this, which makes me think heights are correct
  // LengthKMs height = myTerrainBlockage->getHeightKM(topDegs, centerAzDegs, rangeKMs, outLatDegs, outLonDegs);
  LengthKMs topHeightKMs = Project::attenuationHeightKMs(stationHeightKMs, centerRangeKMs, topDegs);
  LengthKMs c = Project::attenuationHeightKMs(stationHeightKMs, centerRangeKMs, elevDegs);

  // Need elev/range to lat lon here (FIXME: cleaner)
  // These give back same height
  // LengthKMs d = Project::attenuationHeightKMs(stationHeightKMs, centerRangeKMs, bottomDegs);
  // Using bottom of beam to get the terrain height..should be close no matter what 'vertical' angle we use.
  LengthKMs d = getHeightKM(bottomDegs, centerAzDegs, centerRangeKMs, outLatDegs, outLonDegs);
  double aBotTerrainHeightM = myDEMLookup->getValueAtLL(outLatDegs, outLonDegs);

  if (aBotTerrainHeightM == Constants::MissingData) {
    aBotTerrainHeightM = 0;
  }
  LengthKMs TerrainKMs = aBotTerrainHeightM / 1000.0;

  // LengthKMs TerrainKMs = getTerrainHeightKMs(outLatDegs, outLonDegs);

  // Half power radius 'a' from Bech et all 2003, and python libraries
  LengthKMs a = (centerRangeKMs * beamWidthDegs * DEG_TO_RAD) / 2.0;

  ///
  // FIXME: Can do averaging code here for terrain and beam...
  //
  LengthKMs y = TerrainKMs - c;

  // Direct from the Bech Et Al 2003 paper, though it doesn't seem to change the results much,
  // if anything it 'dampens' the terrain effect slightly
  double fractionBlocked = 0.0;

  if (y >= a) {
    fractionBlocked = 1.0;
  } else if (y <= -a) {
    fractionBlocked = 0.0;
  } else { // Partial Beam Blockage PBB calculation
    double a2  = a * a;
    double y2  = y * y;
    double num = (y * sqrt(a2 - y2)) + (a2 * asin(y / a)) + (M_PI * a2 / 2.0); // Part of circle
    double dem = M_PI * a2;                                                    // Full circle
    fractionBlocked = num / dem;                                               // % covered
  }

  // Assign Partial beam blockage final value
  pbb = fractionBlocked;
  if (pbb < 0) { pbb = 0; }
  if (pbb > 1.0) { pbb = 1.0; }

  // Update Cumulative
  if (pbb > cbb) {
    cbb = pbb;
  }

  // Does bottom of beam hit or not?
  hit = (d - TerrainKMs <= myMinTerrainKMs);
} // TerrainBlockage2me::calculatePercentBlocked
