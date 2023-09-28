#pragma once

#include "rFusionCache.h"

/** RAPIO API */
#include <RAPIO.h>

// Nearest 'n' basically.  RAM will explode with larger values
#define FUSION_MAX_CONTRIBUTING 4

namespace rapio {
/** Key to reduce size of nearest grid */
typedef unsigned short SourceIDKey;

/** Store the nearest data.  Size matters here */
class NearestIDs : public Data {
public:

  /** Make sure we're set to the null id and maximum range possible
   * before running the nearest algorithm */
  NearestIDs() : id{0, 0, 0, 0},
    range{std::numeric_limits<float>::max(),
      std::numeric_limits<float>::max(),
      std::numeric_limits<float>::max(),
      std::numeric_limits<float>::max()}
  { }

  // Why not vector? Because it's 24 bytes just for size/capacity
  SourceIDKey id[FUSION_MAX_CONTRIBUTING];
  float range[FUSION_MAX_CONTRIBUTING];
};

/** Store a unique id per source and grid information when reading files from stage1 */
class SourceInfo : public Data {
public:
  SourceIDKey id;
  std::string name;
  size_t startX;
  size_t startY;
  size_t numX;
  size_t numY;
  // Store subgrid mask
  Bitset mask;
};

/**
 * A group idea to handle IO/CPU reduction.  Since we filter out
 * data by nearest 3-4 radars and other factors, we can create
 * a dynamic cache that can tell each stage one if it needs
 * to create data for a point or not.
 * This requires stage ones to generate metainformation, such
 * as a range map.
 *
 * We let stage 1 generate the ranges since this horizontally
 * scales.  It can currently take several seconds to generate
 * the ranges from source center and we don't want roster having
 * to do this math for 300 radars.
 *
 * This range/meta file is read in by scanning/walking the
 * directory.  The nearest neighbor algorithm then inserts each
 * of these into our 'big' data structure.  Finally, we create a 1/0
 * bit mask for each source and write back. This mask is then
 * used by stage1 to turn on/off generated grid points for that
 * source.  The area of coverage for the range and bit masks is
 * the covered subgrid of the CONUS or larger fusion we're running.
 *
 * @author Robert Toomey
 *
 **/
class RAPIOFusionRosterAlg : public rapio::RAPIOAlgorithm {
public:

  /** Create tile algorithm */
  RAPIOFusionRosterAlg(){ };

  /** Declare all algorithm options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(rapio::RAPIOOptions& o) override;

  /** Process a new record/datatype */
  virtual void
  processNewData(rapio::RAPIOData& d) override;

  /** Ingest single cache file */
  void
  ingest(const std::string& sourcename, const std::string& fullpath);

  /** Do nearest neighbor algorithm ingest for a single source.
  * This modifies the current nearest list with the new one. */
  void
  nearestNeighbor(std::vector<FusionRangeCache>& out, size_t id,
    size_t startX, size_t startY, size_t numX, size_t numY);

  /** Scan cache directory looking for range files from stage1 */
  void
  scanCacheDirectory();

  /** Generate masks from nearest neighbor results into individual
   * bit masks for each source */
  void
  generateMasks();

  /** Process heartbeat in subclasses.
   * @param n The actual now time triggering the event.
   * @param p The pinned sync time we're firing for. */
  virtual void
  processHeartbeat(const Time& n, const Time& p) override;

protected:

  /** First time setup */
  void
  firstTimeSetup();

  /** Coordinates for the total merger grid */
  LLCoverageArea myFullGrid;

  /** The full tile/CONUS grid of nearest for a x,y,z location */
  std::vector<NearestIDs> myNearest;

  /** Dimension mapper to get index into myNearest */
  DimensionMapper myNearestIndex;

  /** Map of source names to info used in the nearest list */
  // std::map<std::string, SourceInfo> myNameToInfo;
  std::map<std::string, SourceIDKey> myNameToInfo;

  /** Lookup from id to SourceInfo */
  std::vector<SourceInfo> mySourceInfos;

  /** Sum timer for group operations */
  ProcessTimerSum * myWalkTimer;

  /** Count files/sources processed in a walk */
  size_t myWalkCount;
};
}
