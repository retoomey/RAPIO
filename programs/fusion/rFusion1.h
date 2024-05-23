#pragma once

/** RAPIO API */
#include <RAPIO.h>
#include "rTerrainBlockage.h"
#include "rLLCoverageArea.h"
#include "rVolumeValueResolver.h"
#include "rLLHGridN2D.h"

#include "rFusionCache.h"

namespace rapio {
/** Stage 1 algorithm.  This handles projection and resolving values
 *  for a single source for the final output grid.
 *
 * @author Robert Toomey
 */
class RAPIOFusionOneAlg : public rapio::RAPIOAlgorithm {
public:

  /** Create tile algorithm */
  RAPIOFusionOneAlg() : myDirty(0){ };

  /** Declare all algorithm command line plugins */
  virtual void
  declarePlugins() override;

  /** Declare all algorithm options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(rapio::RAPIOOptions& o) override;

  /** Create the cache of interpolated layers */
  void
  createLLGCache(
    const std::string   & outputName,
    const std::string   & outputUnits,
    const LLCoverageArea& g);

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

  /** Initialization done on first incoming data */
  void
  firstDataSetup(std::shared_ptr<RadialSet> r,
    const std::string& radarName, const std::string& typeName);

  /** Read/update the current range/file we generate for Roster */
  void
  updateRangeFile();

  /** Read/update the current coverage mask generated by Roster */
  void
  readCoverageMask();

  /** Process a new incoming RadialSet */
  void
  processRadialSet(std::shared_ptr<RadialSet> r);

  /** Process a single height layer */
  size_t
  processHeightLayer(size_t             layer,
    const std::vector<double>           levels,
    const std::vector<DataType *>       pointers,
    const std::vector<DataProjection *> projectors,
    const Time                          & rTime,
    std::shared_ptr<VolumeValueIO>      stage2p
  );

  /** Process a volume generating stage 2 output */
  void
  processVolume(const Time rTime);

  /** Process a new record/datatype */
  virtual void
  processNewData(rapio::RAPIOData& d) override;

  /** Process heartbeat in subclasses.
   * @param n The actual now time triggering the event.
   * @param p The pinned sync time we're firing for. */
  virtual void
  processHeartbeat(const Time& n, const Time& p) override;

protected:

  /** Override output params for output */
  std::map<std::string, std::string> myOverride;

  /** Store elevation volume handler */
  std::shared_ptr<Volume> myElevationVolume;

  /** Store terrain blockage */
  std::shared_ptr<TerrainBlockage> myTerrainBlockage;

  /** Store first radar name.  Currently we can only handle 1 radar */
  std::string myRadarName;

  /** Store first type name.  Currently we can only handle 1 typename */
  std::string myTypeName;

  /** Store first radar location.  Currently we can only handle 1 radar */
  LLH myRadarCenter;

  /** Cached lookup for radar to conus grid projection */
  std::vector<std::shared_ptr<AzRanElevCache> > myLLProjections;

  /** Cached lookup for level same */
  std::vector<std::shared_ptr<LevelSameCache> > myLevelSames;

  /** Cached sin/cos lookup.  Needs to be one 2D over radar coverage area */
  std::shared_ptr<SinCosLatLonCache> mySinCosCache;

  /** Coordinates for the total merger grid */
  LLCoverageArea myFullGrid;

  /** The typename we use for stage 2 ingest products */
  std::string myWriteStage2Name;

  /** The typename we use for debug cappis */
  std::string myWriteCAPPIName;

  /** The units we use for all output products */
  std::string myWriteOutputUnits;

  /** Write lat lon subgrid vs full grid (typically radar box vs CONUS) */
  bool myWriteSubgrid;

  /** Apply Lak's moving average radial set prefilter  */
  bool myUseLakSmoothing;

  /** Volume alg */
  std::string myVolumeAlg;

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
  std::shared_ptr<LLHGridN2D> myLLGCache;

  /** The resolver we are using to calculate values */
  std::shared_ptr<VolumeValueResolver> myResolver;

  /** The mask from roster covering current nearest neighbor.
   * Basically we don't need to calculate values currently covered by
   * other radars. */
  Bitset myMask;

  /** Do we have a mask? */
  bool myHaveMask;

  /** How many data files have come in since a process volume? */
  size_t myDirty;

  /** The partition info we're using */
  PartitionInfo myPartitionInfo;
};
}
