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

  /** Do nearest neighbor algorithm */
  void
  nearestNeighbor(std::vector<FusionRangeCache>& out, size_t id,
    size_t startX, size_t startY, size_t numX, size_t numY);

  /** Scan cache directory */
  void
  scanCacheDirectory();

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
