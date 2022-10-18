#include "rTerrainBlockage2me.h"

using namespace rapio;

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

void
TerrainBlockage2me::calculatePercentBlocked(
  // Constants in 3D space information (could be instance)
  LengthKMs stationHeightKMs, AngleDegs beamWidthDegs,
  // Variables in 3D space information.  Should we do center automatically?
  AngleDegs elevDegs, AngleDegs centerAzDegs, LengthKMs centerRangeKMs,
  // Greatest PBB so far
  float& greatestPercentage,
  // Final output percentage for gate
  float& v)
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

  // Logic here: We want CBB for each gate, unless the beam touches the bottom..then
  // it's 100% at that gate.  FIXME: I want to provide all the data at some point let
  // something else determine the blockage logic.  This will be CBB where anywhere the
  // beam bottom touches we have a 100% gate.
  v = fractionBlocked; // Start with PBB
  if (v < 0) { v = 0; }
  if (v > 1.0) { v = 1.0; }

  if (v > greatestPercentage) { // and become CBB
    greatestPercentage = v;
  } else {
    v = greatestPercentage;
  }
  // Either the final gate percentage will be the CBB, or 100% if touching earth:
  if (d - TerrainKMs <= myMinTerrainKMs) { v = 1.0; } // less than min height of 0
}                                                     // TerrainBlockage2me::calculateGate
