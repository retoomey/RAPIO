#include "rTerrainBlockage.h"
#include "rProcessTimer.h"

#include <algorithm>

using namespace rapio;

// Unlike MRMS we don't read from XML.  Could always add this later
// if we want to be able to dynamically change it.
#define MIN_HT_ABOVE_TERRAIN_M 0

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

TerrainBlockage::TerrainBlockage(std::shared_ptr<LatLonGrid> aDEM,
  const LLH                                                  & radarLocation_in,
  const LengthKMs                                            & radarRangeKMs,
  const std::string                                          & radarName)
  :
  myDEM(aDEM),
  myRadarLocation(radarLocation_in),
  myRays(NUM_RAYS)
{
  // Make a lookup for our primary data layer, which is the height array
  myDEMLookup = std::make_shared<LatLonGridProjection>(Constants::PrimaryDataName, myDEM.get());

  // FIXME: I think this work code should be outside the constructor, even if it adds another
  // function call
  std::vector<PointBlockage> terrainPoints;

  // This makes a 2D array of azimuth values the same dimensions as the DEM
  computeTerrainPoints(radarRangeKMs, terrainPoints);

  // More calculation...eh 'maybe' this go all into one function, don't know
  computeBeamBlockage(terrainPoints);

  // Ignoring this for now, doesn't appear to be used, at least for merger
  //  addManualOverrides(radarName);
}

// Entrance point from merger
unsigned char
TerrainBlockage::
computePercentBlocked(
  const AngleDegs& beamWidthDegs,
  const AngleDegs& beamElevationDegs,
  const AngleDegs& binAzimuthDegs,
  const LengthKMs& binRangeKMs) const
{
  // FIXME: Is this ever used elsewhere? If not, we might just do the pinning there.
  float blocked = computeFractionBlocked(beamWidthDegs, beamElevationDegs,
      binAzimuthDegs, binRangeKMs);
  // Ok pin the fraction to 0-100 percent
  int result = int(0.5 + blocked * 100);

  if (result < 0) {
    result = 0;
  } else if (result > 100) {
    result = 100;
  }
  return result;
}

float
TerrainBlockage::
computeFractionBlocked(
  const AngleDegs& beamWidthDegs,
  const AngleDegs& beamElevationDegs,
  const AngleDegs& binAzimuthDegs,
  const LengthKMs& binRangeKMs) const
{
  AngleDegs bottomDegs = beamElevationDegs - 0.5 * beamWidthDegs;
  AngleDegs topDegs    = beamElevationDegs + 0.5 * beamWidthDegs;

  // The bottom of the beam has to clear the terrain by at least 150m
  float htM = getHeightAboveTerrainKM(bottomDegs, binAzimuthDegs, binRangeKMs) * 1000; // to meters

  if (htM < MIN_HT_ABOVE_TERRAIN_M) {
    return 1.0;
  }

  AngleDegs minazDegs = binAzimuthDegs - 0.5 * beamWidthDegs;
  AngleDegs maxazDegs = binAzimuthDegs + 0.5 * beamWidthDegs;

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
    // const RayBlockage& blockers = rays[rayno];
    const std::vector<PointBlockage>& blockers = myRays[rayno];
    const size_t num_blockers = blockers.size();
    for (size_t i = 0; i < num_blockers; ++i) { // auto loop better right?
      const PointBlockage& block = blockers[i];

      // then find the minimum passed through them at this range & elev
      if ( (binRangeKMs > block.startKMs) &&
        (binRangeKMs < block.endKMs) &&
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

  return blocked;
} // TerrainBlockage::computeFractionBlocked

namespace
{
float
power_density(float dist)
{
  // eq. 3.2a of Doviak-Zrnic gives the power density function
  //   theta -- angular distance from the beam axis = dist * beamwidth
  //   sin(theta) is approx theta = dist*beamwidth
  //   lambda (eq.3.2b) = beamwidth*D / 1.27
  //   thus, Dsin(theta)/lambda = D * dist * beamwidth / (D*beamwidth/1.27)
  //                            = 1.27 * dist
  //   and the weight is ( J_2(pi*1.27*dist)/(pi*1.27*dist)^2 )^2
  //      dist is between -1/2 and 1/2, so abs(pi*1.27*dist) < 2
  //   in this range, J_2(x) is approximately 1 - exp(-x^2/8.5)
  float x = M_PI * 1.27 * dist;

  if (x < 0.01) {
    x = 0.01; // avoid divide-by-zero
  }
  float wt = (1 - exp(-x * x / 8.5)) / (x * x);

  wt = wt * wt;
  return (wt);
}
}

float
TerrainBlockage :: findAveragePassed(const std::vector<float>& passed) const
{
  // this beam consists of several "rays" i.e. pencil beams
  // numerically integrate the amounts passed by each of these beams
  float sum_passed = 0;
  float sum_wt     = 0;
  int N       = passed.size();
  float halfN = 0.5 * N;

  for (int i = 0; i < N; ++i) {
    // this is the
    float dist = (i + 0.5 - halfN) / N; // -1/2 to 1/2
    float wt   = power_density(dist);
    sum_passed += passed[i] * wt;
    sum_wt     += wt;
  }
  return (sum_passed / sum_wt);
}

LengthKMs
TerrainBlockage ::
getHeightAboveTerrainKM(const AngleDegs elevDegs,
  const AngleDegs                       azDegs,
  const LengthKMs                       rnKMs) const
{
  // This is what WDSS2 does, but we don't even need the lat lon, km here...
  // FIXME: use the direct approximation Radar formula for this and just put in Project
  AngleDegs outLatDegs, outLonDegs;
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

  if (outLonDegs <= -180) {
    outLonDegs += 360;
  }

  if (outLatDegs > 180) {
    outLatDegs -= 360;
  }

  // Add radar height location to raise beam to correct height
  outHeightKMs += myRadarLocation.getHeightKM();
  double aHeightMeters = myDEMLookup->getValueAtLL(outLatDegs, outLonDegs);

  if (aHeightMeters != -9999) {
    outHeightKMs -= (aHeightMeters / 1000.0);
  }
  return (outHeightKMs);
} // TerrainBlockage::getHeightAboveTerrainKM

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

      if ( (block.elevDegs > GROUND) && (block.startKMs < (radarRangeKMs)) ) {
        terrainPoints.push_back(block); // variable bleh....
        block_i.push_back(i);
        block_j.push_back(j);
      }
    }
  }

  // compute azimuthal spread for each of the blockers
  for (size_t i = 0; i < terrainPoints.size(); ++i) {
    terrainPoints[i].setAzimuthalSpread(aref, block_i[i], block_j[i]);
  }
  LogInfo("Found " << terrainPoints.size() << " terrain blockers.\n");
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
  AngleDegs maxSoFarDegs; // zero

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
  int x, int y)
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

  minRayNumber = getRayNumber(azimuths[x][y] - azimuthalSpread);
  maxRayNumber = getRayNumber(azimuths[x][y] + azimuthalSpread);
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
