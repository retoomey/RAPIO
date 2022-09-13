#include "rTerrainBlockage.h"
#include "rProcessTimer.h"
#include "rRadialSet.h"

#include <algorithm>

using namespace rapio;

// What's interesting so far, is that TerrainBlockage does the whole
// virtual RadialSet overlay thing that RadialSetLookup does to cache values.
// The question is, could we combine these somehow or share a common class or
// something to make cleaner/more efficient?
namespace
{
const size_t NUM_RAYS = 7200;
const float ANGULAR_RESOLUTION = 360.0 / NUM_RAYS;
const double GROUND = 0.1; // degs
};

TerrainBlockageBase::TerrainBlockageBase(std::shared_ptr<LatLonGrid> aDEM,
  const LLH                                                          & radarLocation_in,
  const LengthKMs                                                    & radarRangeKMs,
  const std::string                                                  & radarName,
  LengthKMs                                                          minTerrainKMs)
  : myDEM(aDEM), myRadarLocation(radarLocation_in), myMinTerrainKMs(minTerrainKMs), myMaxRangeKMs(radarRangeKMs)
{
  // Make a lookup for our primary data layer, which is the height array
  myDEMLookup = std::make_shared<LatLonGridProjection>(Constants::PrimaryDataName, myDEM.get());
}

//
// Terrain height and height functions.  Feel like they could be cleaner
//
LengthKMs
TerrainBlockageBase::
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
TerrainBlockageBase ::
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
TerrainBlockageBase ::
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

//
// End terrain height and height functions.
//

float
TerrainBlockageBase::
getPowerDensity(float dist)
{
  float x = M_PI * 1.27 * dist;

  if (x < 0.01) {
    x = 0.01; // avoid divide-by-zero
  }
  float wt = (1 - exp(-x * x / 8.5)) / (x * x);

  wt = wt * wt;
  return (wt);
}

TerrainBlockage2::TerrainBlockage2(std::shared_ptr<LatLonGrid> aDEM,
  const LLH                                                    & radarLocation_in,
  const LengthKMs                                              & radarRangeKMs,
  const std::string                                            & radarName)
  : TerrainBlockageBase(aDEM, radarLocation_in, radarRangeKMs, radarName)
{ }

void
TerrainBlockage2::calculatePercentBlocked(
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
}                                                     // TerrainBlockage2::calculateGate

void
TerrainBlockageBase::calculateTerrainPerGate(std::shared_ptr<RadialSet> rptr)
{
  // FIXME: check radar name?
  // FIXME: We could make a general RadialSet center gate marcher which could be useful.
  RadialSet& rs = *rptr;
  const AngleDegs elevDegs         = rs.getElevationDegs();
  const AngleDegs myBeamWidthDegs  = 1.0; // FIXME
  const LengthKMs stationHeightKMs = rs.getLocation().getHeightKM();

  // Create output array for now on the RadialSet.  Permanently store for moment
  // for debugging or outputting to netcdf, etc.
  auto ta = rs.addFloat2D("TerrainPercent", "Dimensionless", { 0, 1 });
  auto& terrainPercent = ta->ref();

  // First gate distance
  LengthKMs startKMs = rs.getDistanceToFirstGateM() / 1000.0;
  auto& gwMs         = rs.getGateWidthVector()->ref(); // We 'could' check nulls here
  auto& azDegs       = rs.getAzimuthVector()->ref();
  auto& azSpaceDegs  = rs.getAzimuthSpacingVector()->ref();

  // March over RadialSet
  const size_t numRadials = rs.getNumRadials();
  const size_t numGates   = rs.getNumGates();

  for (int r = 0; r < numRadials; ++r) {
    const AngleDegs azDeg        = azDegs[r];
    const AngleDegs centerAzDegs = azDeg + (.5 * azSpaceDegs[r]);
    const LengthKMs gwKMs        = gwMs[r] / 1000.0; // Constant per radial
    LengthKMs rangeKMs = startKMs;

    float greatestPercentage = -1000; // percentage

    for (int g = 0; g < numGates; ++g) {
      LengthKMs centerRangeKMs = rangeKMs + (.5 * gwKMs);

      calculatePercentBlocked(stationHeightKMs, myBeamWidthDegs,
        elevDegs, centerAzDegs, centerRangeKMs,
        greatestPercentage, terrainPercent[r][g]);

      rangeKMs += gwKMs;
    }
  }
} // TerrainBlockageBase::calculateTerrainPerGate

TerrainBlockage::TerrainBlockage(std::shared_ptr<LatLonGrid> aDEM,
  const LLH                                                  & radarLocation_in,
  const LengthKMs                                            & radarRangeKMs,
  const std::string                                          & radarName)
  : TerrainBlockageBase(aDEM, radarLocation_in, radarRangeKMs, radarName),
  myRays(NUM_RAYS),
  myInitialized(false)
{ }

void
TerrainBlockage::initialize()
{
  // Lak's method requires creating a virtual radialset overlay for
  // integrating the blockage within sub 'pencils' of the azimuth coverage area
  if (!myInitialized) {
    std::vector<PointBlockage> terrainPoints;

    // This makes a 2D array of azimuth values the same dimensions as the DEM
    computeTerrainPoints(myMaxRangeKMs, terrainPoints);

    // More calculation...eh 'maybe' this go all into one function, don't know
    computeBeamBlockage(terrainPoints);

    // Ignoring this for now, doesn't appear to be used, at least for merger
    //  addManualOverrides(radarName);

    myInitialized = true;
  }
}

void
TerrainBlockage::calculatePercentBlocked(
  // Constants in 3D space information (could be instance)
  LengthKMs stationHeightKMs, AngleDegs beamWidthDegs,
  // Variables in 3D space information.  Should we do center automatically?
  AngleDegs elevDegs, AngleDegs centerAzDegs, LengthKMs centerRangeKMs,
  // Greatest PBB so far
  float& greatestPercentage,
  // Final output percentage for gate
  float& v)
{
  if (!myInitialized) {
    initialize();
  }

  // Note: So with this cutoff we're not PBB or CBB
  // The bottom of the beam has to clear the terrain by at least myMinTerrainKMs
  AngleDegs bottomDegs = elevDegs - 0.5 * beamWidthDegs;
  LengthMs htKMs       = getHeightAboveTerrainKM(bottomDegs, centerAzDegs, centerRangeKMs);

  if (htKMs < myMinTerrainKMs) {
    v = 1.0;
    return;
  }

  AngleDegs topDegs   = elevDegs + 0.5 * beamWidthDegs;
  AngleDegs minazDegs = centerAzDegs - 0.5 * beamWidthDegs;
  AngleDegs maxazDegs = centerAzDegs + 0.5 * beamWidthDegs;

  size_t startRay = PointBlockage::getRayNumber(minazDegs);
  size_t endRay   = PointBlockage::getRayNumber(maxazDegs);

  if (endRay < startRay) {
    endRay += NUM_RAYS; // unwrap
  }

  const int RESOLUTION = (endRay - startRay) + 1;
  std::vector<float> passed(RESOLUTION, 1.0); // initially 1.0

  for (size_t ray = startRay; ray <= endRay; ++ray) {
    size_t rayno = ray % NUM_RAYS; // re-wrap
    // consider all the blockers along this ray
    const std::vector<PointBlockage>& blockers = myRays[rayno];
    const size_t num_blockers = blockers.size();
    for (size_t i = 0; i < num_blockers; ++i) { // auto loop better right?
      const PointBlockage& block = blockers[i];

      // then find the minimum passed through them at this range & elev
      if ( (centerRangeKMs > block.startKMs) &&
        (centerRangeKMs < block.endKMs) &&
        (block.elevDegs > bottomDegs) )
      {
        // fraction passed ...
        float vertFactor = ( topDegs - block.elevDegs  ) / beamWidthDegs;
        if (vertFactor < 0) {
          vertFactor = 0;
        }
        size_t whichBin = ray - startRay;
        // minimum passed by terrain along path
        if (vertFactor < passed[whichBin]) {
          passed[whichBin] = vertFactor;
        }
      }
    }
  }

  float avgPassed = findAveragePassed(passed);

  float blocked = 1 - avgPassed;

  v = blocked;
} // TerrainBlockage::computeFractionBlocked

float
TerrainBlockage :: findAveragePassed(const std::vector<float>& passed)
{
  // this beam consists of several "rays" i.e. pencil beams
  // numerically integrate the amounts passed by each of these beams
  float sum_passed = 0;
  float sum_wt     = 0;
  int N       = passed.size();
  float halfN = 0.5 * N;

  for (int i = 0; i < N; ++i) {
    float dist = (i + 0.5 - halfN) / N; // -1/2 to 1/2
    float wt   = getPowerDensity(dist);
    sum_passed += passed[i] * wt;
    sum_wt     += wt;
  }
  return (sum_passed / sum_wt);
}

// --------------------------------------------------------------------
// Below here are the creation functions, pre reading making cache
// information

void
TerrainBlockage ::
computeTerrainPoints(
  const LengthKMs           & radarRangeKMs,
  std::vector<PointBlockage>& terrainPoints)
{
  // ProcessTimer("Computing Terran Points for a Single Radar...\n");

  // Basically here, Lak takes the DEM grid and creates a 2D array to store
  // the azimuth value for each point. In RAPIO,  we can add a 2DArray to our DEM
  // using the LatLonGrid dimensions.  This would allow us to permanently cache the
  // calculation we do here back to disk if wanted.  But WDSS2 can't read it since
  // it's multiraster data.

  // keep track of azimuths to compute azimuthal spread
  const size_t numLats = myDEM->getNumLats();
  const size_t numLons = myDEM->getNumLons();

  LogInfo("Computing blockers in "
    << numLats << "x" << numLons
    << " terrain grid.\n");
  auto azimuthsArray = myDEM->addFloat2D("azimuths", "degrees", { 0, 1 });
  auto& aref         = azimuthsArray->ref(); // auto would copy, use auto&.  A bit annoying c++
  auto& href         = myDEM->getFloat2DRef();

  // FIXME: order might matter here for memory accsss...it's in netcdf order
  // I forget at moment.
  // In theory we could speed this stuff up by raw calculating here and not
  // using functions..
  // Though funny this part is actually reasonable fast, just a couple seconds...
  std::vector<int> block_i, block_j; // ok we store this?

  for (size_t i = 0; i < numLats; ++i) {
    for (size_t j = 0; j < numLons; ++j) {
      // FIXME: rapio core replace missing not implemented
      if (href[i][j] == -9999) { href[i][j] = -99999; } // Passed functions later.  Bleh the old terrain badly
      const double& v = href[i][j];

      // We need to fill the azimuth array, so no special skipping here
      // if (v == -9999) { continue; }

      // Get location of center of cell at location.  Then override this
      // this the DEM height to pass to Azmuth/Range conversion
      // Lak used the top left, I think we actually want the center of the cell
      // it probably doesn't matter much
      // FIXME: we 'could' march by spacing from topleft save a ton of math
      // here in the center multiplication. Though adding might 'drift'
      LLH loc = myDEM->getCenterLocationAt(i, j);
      loc.setHeightKM(v / 1000.0); // data is in meters  FIXME: units?

      // Crap Lak does want the block.elev, stores in object..
      // BeamPath_LLHtoAzRangeElev(loc123, radarLocation123,  e, az, rn (meters))
      PointBlockage block;
      Project::BeamPath_LLHtoAzRangeElev(
        // Ok wasted to create the location object right?
        loc.getLatitudeDeg(), loc.getLongitudeDeg(), loc.getHeightKM(),
        myRadarLocation.getLatitudeDeg(), myRadarLocation.getLongitudeDeg(),
        myRadarLocation.getHeightKM(), block.elevDegs, block.azDegs, block.startKMs);

      // Store the azimuth for later use.
      aref[i][j] = block.azDegs;

      if ( (block.elevDegs <= GROUND) ) {
        continue;
      }

      if ( (block.startKMs >= (radarRangeKMs)) ) {
        continue;
      }
      // We actually need negative angles due to refraction drop..the delta heights also bring things back up
      // This is a bug in WDSS2 I think...not having this creates 100 percent blockage which means
      // values farther out get auto clipped.  Most likely this bug hasn't been noticed since with merger the
      // weights drop as you get farther from the radar.
      //     if ( (block.elevDegs > GROUND) && (block.startKMs < (radarRangeKMs)) ) {
      //   if ( block.startKMs < (radarRangeKMs) ) { // don't see difference but do we have any negative angles?

      // We need to check the DEM range actually since setAzimuthSpread looks around the point, we
      // can't handle points directly on the DEM border line..
      if ((i > 0) && (i < numLats - 1) && (j > 0) && (j < numLons - 1)) {
        terrainPoints.push_back(block);
        block_i.push_back(i);
        block_j.push_back(j);
      }
    }
  }

  // compute azimuthal spread for each of the blockers
  for (size_t i = 0; i < terrainPoints.size(); ++i) {
    PointBlockage::setAzimuthalSpread(aref, block_i[i], block_j[i],
      terrainPoints[i].minRayNumber, terrainPoints[i].maxRayNumber);
  }
  LogInfo("Found " << terrainPoints.size() << " terrain blockers.\n");
  LogInfo(
    "Sizes: " << (numLats * numLons) << " - " << 2 * (numLats) + 2 * (numLons - 2) << " == " << terrainPoints.size() <<
      "\n");
} // TerrainBlockage::computeTerrainPoints

void
TerrainBlockage ::
computeBeamBlockage(const std::vector<PointBlockage>& terrainPoints)
{
  LogInfo("Computing beam blockage for "
    << myRays.size() << " rays at azimuthal resolution of "
    << ANGULAR_RESOLUTION << "\n");

  int num_beams_blocked = 0;

  // So O(RAYS*O(N)) terrain points...
  for (size_t i = 0; i < myRays.size(); ++i) {
    float az     = i * ANGULAR_RESOLUTION;
    size_t rayno = PointBlockage::getRayNumber(az);
    for (size_t j = 0; j < terrainPoints.size(); ++j) {
      if (terrainPoints[j].contains(rayno) ) { // This makes length random right?
        myRays[i].push_back(terrainPoints[j]);
      }
    }

    if (myRays[i].size() > 0) {
      ++num_beams_blocked;
    }
  }

  LogInfo("Of the " << NUM_RAYS << " beams, "
                    << num_beams_blocked
                    << " beams are blocked at an elevation above "
                    << GROUND << "\n");


  for (size_t i = 0; i < myRays.size(); ++i) {
    if (myRays[i].size() > 0) {
      pruneRayBlockage(myRays[i]);

      #ifdef DEBUG
      if (i % 20 == 0) {
        LogInfo("Beam blockage at: " << myRays[i].front().az);
        for (size_t j = 0; j < myRays[i].size(); ++j) {
          if (myRays[i][j].elev > EFFECTIVE) { // Uggh vector of vector
            LogInfo(" " << myRays[i][j].rn << "," << myRays[i][j].elev);
          }
        }
        LogInfo("\n");
      }
      #endif
    }
  }
} // TerrainBlockage::computeBeamBlockage

void
TerrainBlockage ::
pruneRayBlockage(std::vector<PointBlockage>& orig)
{
  auto sort_by_range = []( const PointBlockage& a,
    const PointBlockage& b )
    {
      return a.startKMs < b.startKMs;
    };

  std::sort(orig.begin(), orig.end(), sort_by_range);

  std::vector<PointBlockage> result;
  AngleDegs maxSoFarDegs = 0; // zero

  for (size_t i = 0; i < orig.size(); ++i) {
    // elevations lower than what we have already seen don't count ...
    if (orig[i].elevDegs > maxSoFarDegs) {
      maxSoFarDegs = orig[i].elevDegs;
      result.push_back(orig[i]);
    }
  }
  std::swap(orig, result);
}

void
PointBlockage::setAzimuthalSpread(const boost::multi_array<float, 2>& azimuths,
  int x, int y, size_t& minRay, size_t& maxRay)
{
  float max_diff = 0;

  for (int i = x - 1; i <= (x + 1); ++i) {
    for (int j = y - 1; j <= (y + 1); ++j) {
      // consider only the 4-neighborhood
      if ( ((i == x) || (j == y)) ) {
        float diff1 = fabs(azimuths[i][j] - azimuths[x][y]);
        // this handles the case of 359 to 1 -- diff should be 2 degrees
        float diff2 = fabs(360 - diff1);
        float diff  = std::min(diff1, diff2);

        if (diff > max_diff) {
          max_diff = diff;
        }
      }
    }
  }
  float azimuthalSpread = (max_diff / 2.0);

  minRay = getRayNumber(azimuths[x][y] - azimuthalSpread);
  maxRay = getRayNumber(azimuths[x][y] + azimuthalSpread);
}

size_t
PointBlockage::getRayNumber(float start)
{
  if (start < 0) { start += 360; }
  if (start >= 360) { start -= 360; }
  int ray_no = int(0.5 + start / ANGULAR_RESOLUTION);

  if (ray_no < 0) { return 0; }
  if (ray_no >= int(NUM_RAYS) ) { return (NUM_RAYS - 1); }
  return ray_no;
}
