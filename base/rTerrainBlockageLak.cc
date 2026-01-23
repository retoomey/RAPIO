#include "rTerrainBlockageLak.h"
#include "rProcessTimer.h"
#include "rIODataType.h"

#include <algorithm>

using namespace rapio;

// What's interesting so far, is that TerrainBlockage does the whole
// virtual RadialSet overlay thing that RadialSetLookup does to cache values.
namespace
{
const size_t NUM_RAYS = 7200;
const float ANGULAR_RESOLUTION = 360.0 / NUM_RAYS;
};

std::shared_ptr<TerrainBlockage>
TerrainBlockageLak::create(const std::string & params,
  const LLH                                  & radarLocation,
  const LengthKMs                            & radarRangeKMs, // Range after this is zero blockage
  const std::string                          & radarName,
  LengthKMs                                  minTerrainKMs,
  AngleDegs                                  minAngleDegs)
{
  // Try to read DEM from provided info
  std::string myTerrainPath = params;

  if (!myTerrainPath.empty()) {
    myTerrainPath += "/";
  }
  std::string file = myTerrainPath + radarName + ".nc";
  std::shared_ptr<LatLonGrid> aDEM = IODataType::read<LatLonGrid>(file);

  return std::make_shared<TerrainBlockageLak>(TerrainBlockageLak(aDEM, radarLocation, radarRangeKMs, radarName,
           minTerrainKMs, minAngleDegs));
}

TerrainBlockageLak::TerrainBlockageLak(std::shared_ptr<LatLonGrid> aDEM,
  const LLH                                                        & radarLocation_in,
  const LengthKMs                                                  & radarRangeKMs,
  const std::string                                                & radarName,
  const LengthKMs                                                  minTerrainKMs,
  const AngleDegs                                                  minAngleDegs)
  : TerrainBlockage(aDEM, radarLocation_in, radarRangeKMs, radarName, minTerrainKMs, minAngleDegs),
  myRays(NUM_RAYS),
  myInitialized(false)
{ }

void
TerrainBlockageLak::introduceSelf()
{
  std::shared_ptr<TerrainBlockageLak> newOne = std::make_shared<TerrainBlockageLak>();
  Factory<TerrainBlockage>::introduce("lak", newOne);
}

std::string
TerrainBlockageLak::getHelpString(const std::string& fkey)
{
  return "Polar terrain based on Fulton, Breidenbach, Miller and Bannon 1998 and Zhang,Jian,Gourly,Howard 2002.";
}

void
TerrainBlockageLak::initialize()
{
  // Lak's method requires creating a virtual radialset overlay for
  // integrating the blockage within sub 'pencils' of the azimuth coverage area
  if (!myInitialized) {
    std::vector<PointBlockageLak> terrainPoints;

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
TerrainBlockageLak::calculatePercentBlocked(
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
  if (!myInitialized) {
    initialize();
  }

  // Check beamwidth bottom
  AngleDegs bottomDegs = elevDegs - 0.5 * beamWidthDegs;
  LengthMs htKMs       = getHeightAboveTerrainKM(bottomDegs, centerAzDegs, centerRangeKMs);

  hit = (htKMs < myMinTerrainKMs);

  AngleDegs topDegs   = elevDegs + 0.5 * beamWidthDegs;
  AngleDegs minazDegs = centerAzDegs - 0.5 * beamWidthDegs;
  AngleDegs maxazDegs = centerAzDegs + 0.5 * beamWidthDegs;

  size_t startRay = PointBlockageLak::getRayNumber(minazDegs);
  size_t endRay   = PointBlockageLak::getRayNumber(maxazDegs);

  if (endRay < startRay) {
    endRay += NUM_RAYS; // unwrap
  }

  const int RESOLUTION = (endRay - startRay) + 1;
  std::vector<float> passed(RESOLUTION, 1.0); // initially 1.0

  for (size_t ray = startRay; ray <= endRay; ++ray) {
    size_t rayno = ray % NUM_RAYS; // re-wrap
    // consider all the blockers along this ray
    const std::vector<PointBlockageLak>& blockers = myRays[rayno];
    const size_t num_blockers = blockers.size();
    for (size_t i = 0; i < num_blockers; ++i) { // auto loop better right?
      const PointBlockageLak& block = blockers[i];

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

  // Lak is just calculating the cumulative or final value.
  // I'm not sure how to get partial for his method, so we'll
  // set them the same here for now.
  cbb = blocked;
  pbb = blocked;
} // TerrainBlockage::computeFractionBlocked

float
TerrainBlockageLak :: findAveragePassed(const std::vector<float>& passed)
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
TerrainBlockageLak ::
computeTerrainPoints(
  const LengthKMs              & radarRangeKMs,
  std::vector<PointBlockageLak>& terrainPoints)
{
  // If no DEM info, no blockers
  if (myDEM == nullptr) {
    return;
  }

  // ProcessTimer("Computing Terrain Points for a Single Radar...\n");

  // Basically here, Lak takes the DEM grid and creates a 2D array to store
  // the azimuth value for each point. In RAPIO,  we can add a 2DArray to our DEM
  // using the LatLonGrid dimensions.  This would allow us to permanently cache the
  // calculation we do here back to disk if wanted.  But WDSS2 can't read it since
  // it's multiraster data.

  // keep track of azimuths to compute azimuthal spread
  const size_t numLats = myDEM->getNumLats();
  const size_t numLons = myDEM->getNumLons();

  fLogInfo("Computing blockers in {}x{} terrain grid.", numLats, numLons);
  auto azimuthsArray = myDEM->addFloat2D("azimuths", "degrees", { 0, 1 });
  auto& aref         = azimuthsArray->ref(); // auto would copy, use auto&.  A bit annoying c++
  auto& href         = myDEM->getFloat2DRef();

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

      PointBlockageLak block;
      Project::BeamPath_LLHtoAzRangeElev(
        // Ok wasted to create the location object right?
        loc.getLatitudeDeg(), loc.getLongitudeDeg(), loc.getHeightKM(),
        myRadarLocation.getLatitudeDeg(), myRadarLocation.getLongitudeDeg(),
        myRadarLocation.getHeightKM(), block.elevDegs, block.azDegs, block.startKMs);

      // Store the azimuth for later use.
      aref[i][j] = block.azDegs;

      if ( (block.elevDegs <= myMinAngleDegs) ) {
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
    PointBlockageLak::setAzimuthalSpread(aref, block_i[i], block_j[i],
      terrainPoints[i].minRayNumber, terrainPoints[i].maxRayNumber);
  }
  fLogInfo("Found {} terrain blockers.", terrainPoints.size());
} // TerrainBlockageLak::computeTerrainPoints

void
TerrainBlockageLak ::
computeBeamBlockage(const std::vector<PointBlockageLak>& terrainPoints)
{
  fLogInfo("Computing beam blockage for {} rays at azimuthal resolution of {}",
    myRays.size(), ANGULAR_RESOLUTION);

  int num_beams_blocked = 0;

  // So O(RAYS*O(N)) terrain points...
  for (size_t i = 0; i < myRays.size(); ++i) {
    float az     = i * ANGULAR_RESOLUTION;
    size_t rayno = PointBlockageLak::getRayNumber(az);
    for (size_t j = 0; j < terrainPoints.size(); ++j) {
      if (terrainPoints[j].contains(rayno) ) { // This makes length random right?
        myRays[i].push_back(terrainPoints[j]);
      }
    }

    if (myRays[i].size() > 0) {
      ++num_beams_blocked;
    }
  }

  fLogInfo("Of the {} beams, {} beams are blocked at an elevation above {}",
    NUM_RAYS, num_beams_blocked, myMinAngleDegs);

  for (size_t i = 0; i < myRays.size(); ++i) {
    if (myRays[i].size() > 0) {
      pruneRayBlockage(myRays[i]);

      #ifdef DEBUG
      if (i % 20 == 0) {
        fLogInfo("Beam blockage at: {}", myRays[i].front().az);
        for (size_t j = 0; j < myRays[i].size(); ++j) {
          if (myRays[i][j].elev > EFFECTIVE) { // Uggh vector of vector
            fLogInfo(" {},{}", myRays[i][j].rn, myRays[i][j].elev);
          }
        }
      }
      #endif
    }
  }
} // TerrainBlockageLak::computeBeamBlockage

void
TerrainBlockageLak ::
pruneRayBlockage(std::vector<PointBlockageLak>& orig)
{
  auto sort_by_range = []( const PointBlockageLak& a,
    const PointBlockageLak& b )
    {
      return a.startKMs < b.startKMs;
    };

  std::sort(orig.begin(), orig.end(), sort_by_range);

  std::vector<PointBlockageLak> result;
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
PointBlockageLak::setAzimuthalSpread(const boost::multi_array<float, 2>& azimuths,
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
PointBlockageLak::getRayNumber(float start)
{
  if (start < 0) { start += 360; }
  if (start >= 360) { start -= 360; }
  int ray_no = int(0.5 + start / ANGULAR_RESOLUTION);

  if (ray_no < 0) { return 0; }
  if (ray_no >= int(NUM_RAYS) ) { return (NUM_RAYS - 1); }
  return ray_no;
}
