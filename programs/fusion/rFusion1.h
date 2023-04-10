#pragma once

/** RAPIO API */
#include <RAPIO.h>
#include "rTerrainBlockage.h"
#include "rLLCoverageArea.h"
#include "rVolumeValueResolver.h"

/**  Use 8 bits for FusionKey, with a max value of 255.  This means our volume
 * tilts need to be limited to 254 (0 key is no value) at a time in memory. This
 * is usually fine for a single moment.
 * This can be increased here safely if needed but it will cost more RAM. */
typedef uint8_t FusionKey;

namespace rapio {
/** So many caches clean up needs to happen.  We need a single one of these per radar center and 2D coverage */
class SinCosLatLonCache : public Utility
{
public:
  SinCosLatLonCache(size_t numX, size_t numY) :
    myNumX(numX), myNumY(numY), mySinGcdIR(numX * numY), myCosGcdIR(numX * numY), myAt(0)
  { }

  /** Return index into 2D lookup */
  inline size_t
  getIndex(size_t x, size_t y)
  {
    return (y * myNumX) + x;
  }

  /** Reset iterator functions */
  inline void
  reset()
  {
    myAt = 0;
  }

  /** Add at current position (a bit faster).  FIXME: could make an iterator */
  inline void
  add(const double sinGcdIR, const double cosGcdIR)
  {
    mySinGcdIR[myAt] = sinGcdIR;
    myCosGcdIR[myAt] = cosGcdIR;
    myAt++;
  }

  /** Get at current position (a bit faster) */
  inline void
  get(double& sinGcdIR, double& cosGcdIR)
  {
    sinGcdIR = mySinGcdIR[myAt];
    cosGcdIR = myCosGcdIR[myAt];
    myAt++;
  }

  /** The X size of our cache */
  size_t myNumX;

  /** The Y size of our cache */
  size_t myNumY;

  /** Current location for raw iteration */
  size_t myAt;

  /** Cached sin */
  std::vector<double> mySinGcdIR;

  /** Cached cos */
  std::vector<double> myCosGcdIR;
};

/** This is the trick, the lightbulb combining Lak's big brain with my slightly smaller one.
 * The radar Lat lon height doesn't change, and the grid of Lat lon height (conus output)
 * we project to is constant during the run.  So we can cache all projection math for this
 * that is calculating the virtual azimuth, elevation and range from radar center to
 * a lat lon height point in our output grid.
 *
 * Basically the entire output of BeamPath_LLHtoAzRangeElev.
 *
 * Also, Terrain percentage uses the same az, range, elev.  The difference here though
 * is that the calculatePercentBlocked needs the AzimuthSpacing.  So technicaly we
 * will need to cache it per unique AzimuthSpacing.
 *
 * This would be per _RADAR_ and _OUTPUT_ grid.  So for N moments or incoming tilts this
 * can be shared.  Awesome.
 *
 * Tried using a 3D array of cache objects with pointer math and still slower than this,
 * interesting. It might be that a set of 2D caches better in RAM vs 3D due to L3, etc.
 */
class AzRanElevCache : public Utility
{
public:

  /** Create lookup of given size */
  AzRanElevCache(size_t numX, size_t numY) :
    myNumX(numX), myNumY(numY),
    myAzimuths(numX * numY), myVirtualElevations(numX * numY), myRanges(numX * numY),
    myTerrainAzimuthSpacing(1), myUpperElev(numX * numY), myLowerElev(numX * numY),
    myNextUpperElev(numX * numY), myNextLowerElev(numX * numY),
    myAt(0)
  { }

  /** Return index into 2D lookup */
  inline size_t
  getIndex(size_t x, size_t y)
  {
    return (y * myNumX) + x;
  }

  /** Store cache lookup given x and y */
  inline void
  set(size_t x, size_t y, const AngleDegs inAzDegs, const AngleDegs inElevDegs, const LengthKMs inRanges)
  {
    const size_t index = getIndex(x, y);

    myAzimuths[index] = inAzDegs;
    myVirtualElevations[index] = inElevDegs;
    myRanges[index] = inRanges;
  }

  /** Store upper/lower markers. Return true if it changes.  When these change, we recalculate data values */
  inline bool
  set(size_t x, size_t y, DataType * lower, DataType * upper, DataType * nextLower, DataType * nextUpper)
  {
    const size_t index = getIndex(x, y);

    bool changed = false;

    const FusionKey lowerKey = (lower == nullptr) ? 0 : lower->getID();

    if (myLowerElev[index] != lowerKey) {
      myLowerElev[index] = lowerKey;
      changed = true;
    }
    const FusionKey upperKey = (upper == nullptr) ? 0 : upper->getID();

    if (myUpperElev[index] != upperKey) {
      myUpperElev[index] = upperKey;
      changed = true;
    }
    const FusionKey nextLowerKey = (nextLower == nullptr) ? 0 : nextLower->getID();

    if (myNextLowerElev[index] != nextLowerKey) {
      myNextLowerElev[index] = nextLowerKey;
      changed = true;
    }
    const FusionKey nextUpperKey = (nextUpper == nullptr) ? 0 : nextUpper->getID();

    if (myNextUpperElev[index] != nextUpperKey) {
      myNextUpperElev[index] = nextUpperKey;
      changed = true;
    }
    return changed;
  }

  /** Reset iterator functions */
  inline void
  reset()
  {
    myAt = 0;
  }

  /** Add at current position (a bit faster).  FIXME: could make an iterator */
  inline void
  add(const AngleDegs inAzDegs, const AngleDegs inElevDegs, const LengthKMs inRanges)
  {
    myAzimuths[myAt] = inAzDegs;
    myVirtualElevations[myAt] = inElevDegs;
    myRanges[myAt] = inRanges;
    myAt++;
  }

  /** Get at current position (a bit faster) */
  inline void
  get(AngleDegs azimuthSpacing, AngleDegs& outAzDegs, AngleDegs& outElevDegs, LengthKMs& outRanges)
  {
    outAzDegs   = myAzimuths[myAt];
    outElevDegs = myVirtualElevations[myAt];
    outRanges   = myRanges[myAt];
    myAt++;
  }

  /** Cache lookup given x and y */
  inline void
  get(size_t x, size_t y, AngleDegs azimuthSpacing, AngleDegs& outAzDegs, AngleDegs& outElevDegs, LengthKMs& outRanges)
  {
    const size_t index = getIndex(x, y);

    outAzDegs   = myAzimuths[index];
    outElevDegs = myVirtualElevations[index];
    outRanges   = myRanges[index];
  }

public:

  /** The X size of our cache */
  size_t myNumX;

  /** The Y size of our cache */
  size_t myNumY;

  /** Cached azimuth degrees for each cell */
  std::vector<AngleDegs> myAzimuths;

  /** Cached virtual elevation degrees for each cell */
  std::vector<AngleDegs> myVirtualElevations;

  // FIXME: meters better here maybe?
  /** Cached range kilometers for each cell */
  std::vector<LengthKMs> myRanges;

  /** Store the terrain azimuth spacing for our cached terrain */
  AngleDegs myTerrainAzimuthSpacing;

  /** Cached last upper elevation for this x, y.  This is only important as a number */
  std::vector<FusionKey> myUpperElev;

  /** Cached last lower elevation for this x, y.  This is only important as a number */
  std::vector<FusionKey> myLowerElev;

  /** Cached last next upper elevation for this x, y.  This is only important as a number */
  std::vector<FusionKey> myNextUpperElev;

  /** Cached last lower elevation for this x, y.  This is only important as a number */
  std::vector<FusionKey> myNextLowerElev;

  /** Current location for raw iteration */
  size_t myAt;
};

/** Basically Implement a LatLonHeightGrid as a collection of 2D layers vs a 3D cube.  In some
 * cases this seems faster vs a 3D array.
 * FIXME: I think I could have two implementations in the core with an option to
 * toggle which is used.  So requesting a LatLonGrid from it would either copy, create a 'view'
 * or just return the 2D LatLonGrid, etc.
 */
class LatLonGridSet : public Data // could also template/generalize to shared_ptr set
{
public:
  /** Get the size */
  size_t size(){ return myGrids.size(); }

  /** Add grid */
  void add(std::shared_ptr<LatLonGrid> a){ myGrids.push_back(a); }

  /** Return grid number i */
  std::shared_ptr<LatLonGrid> get(size_t i){ return myGrids[i]; }

  /** The set of LatLonGrids */
  std::vector<std::shared_ptr<LatLonGrid> > myGrids;
};

class RAPIOFusionOneAlg : public rapio::RAPIOAlgorithm {
public:

  /** Create tile algorithm */
  RAPIOFusionOneAlg(){ };

  /** Declare all algorithm options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;

  /** Declare all algorithm command line plugins */
  virtual void
  declarePlugins() override;

  /** Process all algorithm options */
  virtual void
  processOptions(rapio::RAPIOOptions& o) override;

  /** Create an LLG for our output or caching */
  std::shared_ptr<LatLonGrid>
  createLLG(
    const std::string   & outputName,
    const std::string   & outputUnits,
    const LLCoverageArea& g,
    const LengthKMs     layerHeightKMs);

  /** Create the cache of interpolated layers */
  void
  createLLGCache(
    const std::string& outputName,
    const std::string& outputUnits,
    const LLCoverageArea& g, const std::vector<double>& heightsM);

  // Projection requirements:
  // 1.  Project from the coverage grid (Lat, Lon, Height) specified by CONUS, etc. to a virtual az, range, elev
  //     which is virtual polar.
  //
  /** Create projection cache, Lat Lon Height to virtual azimuth, elev, height lookup per grid location */
  void
  createLLHtoAzRangeElevProjection(
    AngleDegs cLat, AngleDegs cLon, LengthKMs cHeight,
    LLCoverageArea& g);

  /** Write a current layer as a unique full file.  Netcdf or mrms binary files which
   * represent a single radar stage one w2merger equivalent.  This wouldn't be used in
   * operations since it won't stage for N radars, but it's handy for testing resolvers
   * at the stage one level. */
  void
  writeOutputCAPPI(std::shared_ptr<LatLonGrid> output);

  /** Create the old style raw file */
  std::shared_ptr<RObsBinaryTable>
  createRawEntries(AngleDegs elevDegs,
    AngleDegs cLat, AngleDegs cLon, LengthKMs cHeight,
    const std::string& aTypeName, const std::string& aUnits,
    const time_t dataTime);

  /** Add entry to old style raw file
   * FIXME: doesn't work with w2merger stage2. We're leaning towards netcdf method instead for now,
   * this just keeps the code cleaner */
  void
  addRawEntry(const VolumeValue& vv, const Time& time, RObsBinaryTable& t, size_t x, size_t y, size_t layer);

  /** Process a new record/datatype */
  virtual void
  processNewData(rapio::RAPIOData& d) override;

protected:

  /** Override output params for output */
  std::map<std::string, std::string> myOverride;

  // These could probably be unique ptrs

  /** Store first radar name.  Currently we can only handle 1 radar */
  // std::shared_ptr<ElevationVolume> myElevationVolume;
  std::shared_ptr<Volume> myElevationVolume;

  /** Store terrain blockage */
  std::shared_ptr<TerrainBlockage> myTerrainBlockage;

  /** Store first radar name.  Currently we can only handle 1 radar */
  std::string myRadarName;

  /** Store first type name.  Currently we can only handle 1 typename */
  std::string myTypeName;

  /** Heights we do */
  std::vector<double> myHeightsM;

  /** Height index for the height.  Only needed for debugging/sparse height output */
  std::vector<size_t> myHeightsIndex;

  /** Cached lookup for radar to conus grid projection */
  std::vector<std::shared_ptr<AzRanElevCache> > myLLProjections;

  /** Cached sin/cos lookup.  Needs to be one 2D over radar coverage area */
  std::shared_ptr<SinCosLatLonCache> mySinCosCache;

  /** Coordinates for the total merger grid */
  LLCoverageArea myFullGrid;

  /** The typename we use for all output products */
  std::string myWriteOutputName;

  /** The units we use for all output products */
  std::string myWriteOutputUnits;

  /** Write lat lon grids to output */
  bool myWriteLLG;

  /** Write lat lon subgrid vs full grid (typically radar box vs CONUS) */
  bool myWriteSubgrid;

  /** Apply Lak's moving average radial set prefilter  */
  bool myUseLakSmoothing;

  /** Resolver alg */
  std::string myResolverAlg;

  /** Volume alg */
  std::string myVolumeAlg;

  /** Terrain alg */
  std::string myTerrainAlg;

  /** Radar range */
  LengthKMs myRangeKMs;

  /** Coordinates for the one radar we're watching */
  LLCoverageArea myRadarGrid;

  /** A cached single full grid 2D layer for full conus output.  We typically
   * run this with a very large 3D grid, and some things like the web page require
   * files that reference this full grid */
  std::shared_ptr<LatLonGrid> myFullLLG;

  /** Throttle skip counter to avoid IO spamming during testing.
  * The good news is we're too fast for the current pipeline. */
  size_t myThrottleCount;

  /** Cached set of LatLonGrids */
  LatLonGridSet myLLGCache;
};
}
