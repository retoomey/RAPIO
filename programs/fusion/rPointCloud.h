#pragma once

/** RAPIO API */
#include <RAPIO.h>
#include "rTerrainBlockage.h"

namespace rapio {
/** Bin a Lat,Lon,Height point cloud into a X,Y,Z grid defined.
 *  So basically we want a list of contained points for each X,Y.
 *  This is like Blend3D say in merger..but I'm storing just an index
 *  to an exterior binary table array, so not bothering with sparse.
 *
 *  @author Robert Toomey.
 *  This allowed us to perform some action such as cressman on the set of
 *  points that fall within each grid call.
 *  Example: 10 points, index of 0,1,2,3,...9
 *  Add to boxes you get x1,y1,z1  --> 0,3,6,8
 *                       x2,y2,z2  --> 1,2,4,5,7
 *                       (grid box)    (index into original point vector)
 *  FIXME: Might be useful as a generic classed based on DimensionMapper?
 **/
class GridPointsLookup
{
public:
  GridPointsLookup(size_t xdim, size_t ydim, size_t zdim)
  {
    xmax    = xdim;
    ymax    = ydim;
    zmax    = zdim;
    horsize = ydim * zdim;
    // Interesting have to do this
    const size_t fullSize = (xdim * ydim * zdim);

    data.reserve(fullSize);
    for (size_t i = 0; i < fullSize; i++) {
      data.push_back(std::vector<size_t>());
    }
  }

  /** Size of our grid */
  size_t size(){ return xmax * ymax * zmax; }

  /** Get reference to the points in grid cell */
  std::vector<size_t>&
  getPoints(size_t i)
  {
    return data[i];
  }

  /** Store a reference to index in grid spot */
  void
  add(size_t x, size_t y, size_t z, size_t index)
  {
    // For speed, not checking the bounds
    const size_t i = (x * horsize + y * zmax + z);

    // if ( i < xmax*ymax*zmax){
    data[i].push_back(index);
    // }
  }

  /** Get a reference to index in grid spot */
  const std::vector<size_t>&
  getPointsAt(size_t x, size_t y, size_t z)
  {
    const size_t i = (x * horsize + y * zmax + z);

    return data[i];
  }

  /** Dump out positive points for debugging */
  void
  dump()
  {
    for (size_t i = 0; i < xmax * ymax * zmax; i++) {
      const size_t ss = data[i].size();
      if (ss > 0) {
        fLogSevere("{}: size of {}", i, ss);
      }
    }
  }

  /** Clear the point grid of all point references */
  void
  clear()
  {
    for (size_t i = 0; i < xmax * ymax * zmax; i++) {
      data[i] = std::vector<size_t>();
    }
  }

private:
  // Size of xdim*ydim*zdim
  // where each vector is variable length storing index reference
  // to point falling into the xyz 'bin'
  std::vector<std::vector<size_t> > data;

  /** X dimensions of grid */
  size_t xmax;
  /** Y dimensions of grid */
  size_t ymax;
  /** Z dimensions of grid */
  size_t zmax;

  /** Avoids multiplying */
  size_t horsize;
};

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

  /** Buffer temp vectors to RadialSet */
  void
  bufferToRadialSet(std::shared_ptr<RadialSet> r);

  /** Process a new incoming RadialSet */
  void
  processRadialSet(std::shared_ptr<RadialSet> r);

  /** Write the current collected data.  This clears all buffered
   * data and starts gathering new information */
  void
  writeCollectedData(const Time rTime);

  /** Coordinates for the total merger grid */
  LLCoverageArea myFullGrid;

  /** Lookup grid for binning points to a final grid */
  std::shared_ptr<GridPointsLookup> myGridLookup;

  /** Store collected data in a generic DataGrid */
  std::shared_ptr<DataGrid> myCollectedData;

  /** Store elevation volume handler */
  std::shared_ptr<Volume> myElevationVolume;

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

  /** Output after every tilt, not on time range */
  bool myEveryTilt;
};
}
