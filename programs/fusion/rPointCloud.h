#pragma once

/** RAPIO API */
#include <RAPIO.h>
#include "rTerrainBlockage.h"

namespace rapio {
/*
 * A direct polar to radial to lat lon grid sampling algorithm.
 * This differs from 'regular' fusion in that the grid is not static,
 * which would make 'merging' difficult since lat, lon, height
 * is dynamic.
 *
 * This algorihm focuses on appending tables of data generated
 * from a gate sample of a RadialSet.
 *
 * @author Robert Toomey
 **/
class RAPIOPointCloudAlg : public rapio::RAPIOAlgorithm {
public:

  /** Create tile algorithm */
  RAPIOPointCloudAlg() : myDirty(0){ };

  /** Declare all algorithm command line plugins */
  virtual void
  declarePlugins() override;

  /** Declare all algorithm options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(rapio::RAPIOOptions& o) override;

  /** Process a new record/datatype */
  virtual void
  processNewData(rapio::RAPIOData& d) override;

  /** Process heartbeat in subclasses.
   * @param n The actual now time triggering the event.
   * @param p The pinned sync time we're firing for. */
  virtual void
  processHeartbeat(const Time& n, const Time& p) override;

protected:

  /** Initialization done on first incoming data */
  void
  firstDataSetup(std::shared_ptr<RadialSet> r,
    const std::string& radarName, const std::string& typeName);

  /** Process a new incoming RadialSet */
  void
  processRadialSet(std::shared_ptr<RadialSet> r);

  /** Write the current collected data.  This clears all buffered
   * data and starts gathering new information */
  void
  writeCollectedData(const Time rTime);

  /** Store collected data in a generic DataGrid */
  std::shared_ptr<DataGrid> myCollectedData;

  /** Store terrain blockage algorithm used */
  std::shared_ptr<TerrainBlockage> myTerrainBlockage;

  /** Store first radar name.  Currently we can only handle 1 radar */
  std::string myRadarName;

  /** Store first type name.  Currently we can only handle 1 typename */
  std::string myTypeName;

  /** Store first radar location.  Currently we can only handle 1 radar */
  LLH myRadarCenter;

  /** The typename we use for stage 2 ingest products */
  std::string myWriteStage2Name;

  /** The units we use for all output products */
  std::string myWriteOutputUnits;

  /** Radar range */
  LengthKMs myRangeKMs;

  /** Throttle skip counter to avoid IO spamming during testing.
  * The good news is we're too fast for the current pipeline. */
  size_t myThrottleCount;

  /** How many data files have come in since a process volume? */
  size_t myDirty;

  // Table columns: FIXME: Could put all into a class, but for now
  // we will just have a vector per 'column'
  // value, latitude, longitude, height, ux, uy, uz

  /** Values for row */
  std::vector<float> myValues;

  /** Calculated latitude for row */
  std::vector<AngleDegs> myLatDegs;

  /** Calculated longitude for row */
  std::vector<AngleDegs> myLonDegs;

  /** Calculated height for row */
  std::vector<float> myHeightsKMs;

  /** Calculated ux for row */
  std::vector<float> myXs;

  /** Calculated uy for row */
  std::vector<float> myYs;

  /** Calculated uz for row */
  std::vector<float> myZs;
};
}
