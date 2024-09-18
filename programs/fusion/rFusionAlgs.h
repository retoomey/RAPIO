#pragma once

/** RAPIO API */
#include <RAPIO.h>

#include <vector>

namespace rapio {
/** Echo top logic, for the moment */

/** Interpolate/Extrapolate, based upon the
 * rate of change between two valid dbZ values and a height range between them.
 * Basically to interpolate/extrapolate we need to check a few things:
 * One, if the dbZ values are too close, the extrapolation goes to infinity.  Avoid that.
 * FIXME: Maybe we should have a maximum height diff between values
 */
static inline float
inter(float heightAt, float heightLast, float dBZAt, float dBZLast, float threshold)
{
  // Height difference
  const float hdiff = heightLast - heightAt;

  // dBZ absolute difference
  float p = dBZAt - dBZLast;

  if (p < 0) { p = -p; }

  float frac;

  if (p > .01) {
    // If dbz far enough apart in value, interpolate/extrapolate
    frac = (dBZAt - threshold) / p * hdiff;

    // And always make it a positive change in height, basically rate of change
    if (frac < 0) { frac = -frac; }
  } else {
    // If dbz are too close, interpolation goes to infinity so forget it.
    frac = 0.0;
  }
  return frac;
}

// Here we put our 'searching' echotop logic that scans a height
// range for a threshold value.
//
static inline float
doEchoTop(
  const LatLonHeightGrid& gridin,
  const float threshold,
  const std::vector<float>& hlevels,
  const size_t numhts, const size_t i, const size_t j)
{
  // We use all heights ignoring anything from NSE
  const int lower  = 0;
  const int higher = numhts - 1;

  // Keep latest 'hit' height value from top down...
  float lastH   = -10000.0;
  float lastdBZ = 0;

  // Data value (height) we we'll set
  float h = Constants::DataUnavailable;

  // FIXME: Inefficient port at moment.  Probably should just
  // pass in the array of interest
  auto& grid = gridin.getFloat3DRef();

  // Loop from top height to bottom....
  for (int k = higher; k >= lower; --k) {
    const float dBZAt = grid[k][i][j]; // Maybe  > thresh

    // Skip on unavailable data...
    if (dBZAt == Constants::DataUnavailable) {
      continue;
    }
    // ...otherwise missing is the new default data value...
    h = Constants::MissingData;

    // If data value greater than or equal to threshold
    // Note isGood is implicit here if threshold not -99990 something...
    if (dBZAt >= threshold) {
      // -------------------------------------------------------------------------
      // If there was a higher up data value, we can interpolate up to that one
      //
      if (lastH > -1000) {
        h = hlevels[k] + inter(hlevels[k], lastH, dBZAt, lastdBZ, threshold);
        // see if web page is clipping higher values, pin to max height of cube
        if (h > hlevels[higher]) { h = hlevels[higher]; }
        h /= 1000.0; // Convert to kilometers

        // -------------------------------------------------------------------------
        // ...otherwise look downward for one to interpolate with
        //
      } else {
        // Lak always just used the value if nothing above us...
        h  = hlevels[k];
        h /= 1000.0;

        /*
         *          // Downward interpolation search...
         *          // Default to missing unless we find one below.  Thus a single dBZ value
         *          // with all higher and lower heights being missing will be ignored.
         *          // Darrel says this would be most likely a bad dBZ value.
         *          h = Constants::MissingData;
         *
         *          for(int b=k-1;b >=lower; --b){
         *            const float belowDbz = grid[b][i][j];
         *
         *            // Found a good value...ok interpolate to it then and stop
         *            if (Constants::isGood(belowDbz)){
         *
         *              h = hlevels[k]+inter(hlevels[k], hlevels[b], dBZAt, belowDbz, threshold);
         *              h /= 1000.0; // Convert to kilometers
         *              break; // b loop
         *            }
         *          }
         */
      }
      // -------------------------------------------------------------------------

      break; // k loop
    } else {
      // Keep a good value for next pass
      if (Constants::isGood(dBZAt)) {
        lastH   = hlevels[k];
        lastdBZ = dBZAt;
      }
    }
  }
  return h;
} // doEchoTop

/** Do a echo bottom search from the ground up.  Depth uses this */
static inline float
doEchoBottom(
  const LatLonHeightGrid& gridin,
  const float threshold,
  const std::vector<float>& hlevels,
  const size_t numhts, const size_t i, const size_t j)
{
  // We use all heights ignoring anything from NSE
  const int lower  = 0;
  const int higher = numhts - 1;

  // Keep latest 'hit' height value from bottom up...
  float lastH   = -10000.0;
  float lastdBZ = 0;

  // Data value (height) we we'll set
  float h = Constants::DataUnavailable;

  auto& grid = gridin.getFloat3DRef();

  // Loop from bottom height to top....
  for (int k = lower; k <= higher; ++k) {
    const float dBZAt = grid[k][i][j];

    // Skip on unavailable data...
    if (dBZAt == Constants::DataUnavailable) {
      continue;
    }
    // ...otherwise missing is the new default data value...
    h = Constants::MissingData;

    if (dBZAt >= threshold) {
      // -------------------------------------------------------------------------
      // If there was a lower down data value, we can interpolate down to that one
      //
      // ... and interpolate to the first available previous height if we had one.
      if (lastH > -1000) {
        h  = hlevels[k] - inter(hlevels[k], lastH, dBZAt, lastdBZ, threshold);
        h /= 1000.0; // Convert to kilometers

        // -------------------------------------------------------------------------
        // ...otherwise look downward for one to interpolate with
        //
      } else {
        // Lak always just used the value if nothing above us...
        h  = hlevels[k];
        h /= 1000.0;

        /*
         *          // Default to missing unless we find one above.
         *          h = Constants::MissingData;
         *
         *          for(int u=k+1;u <=higher; ++u){
         *            const float aboveDbz = grid[u][i][j];
         *
         *            // Found a good value...ok interpolate to it then and stop
         *            if (Constants::isGood(aboveDbz)){
         *
         *              h = hlevels[k]-inter(hlevels[k], hlevels[u], dBZAt, aboveDbz, threshold);
         *              h /= 1000.0; // Convert to kilometers
         *              break; // b loop
         *            }
         *          }
         */
      }
      // -------------------------------------------------------------------------

      break; // k loop
    } else {
      // Keep a good value for next pass
      if (Constants::isGood(dBZAt)) {
        lastH   = hlevels[k];
        lastdBZ = dBZAt;
      }
    }
  }
  return h;
} // doEchoBottom

/** Start of volume array.
 * FIXME: Need to sync with ArrayAlgorihm at some point.
 * OR humm I feel like all these algorithms could be a list of RAPIOAlgorithms
 * which would allow chaining and/or standalone binaries.
 * Part of this is because we can want to run these algorithms in a few ways:
 * 1.  Directly from stage2 (3D cube kept in RAM)
 * 2.  As a 'stage 3' or FusionAlgs
 * 3.  Maybe as individual RAPIOAlgorithms taking a 3D cube.
 *
 * I'm not sure with modern linux how much 1 matters, since most likely the
 * 3D cube will be in the disk/ram cache in linux and rereading cost woudl be
 * minimum.  We'll test various ways and work to improve the algorithm design
 * and speed.
 *
 * */
class ArrayAlgorithm3D : public Utility {
public:
  /** Create an array algorithm 3d */
  ArrayAlgorithm3D(){ };

  /** Method for processing the volume */
  virtual void
  processVolume(std::shared_ptr<LatLonHeightGrid> input, RAPIOAlgorithm * p) = 0;

  virtual
  ~ArrayAlgorithm3D() = default;
};

/** Create the VIL algorithm.  We'll eventually need NSE and the fusion.xml
 * VIL : public RAPIOAlgorithm */
class VIL : public ArrayAlgorithm3D
{
public:
  virtual void
  processVolume(std::shared_ptr<LatLonHeightGrid> input, RAPIOAlgorithm * p) override;

  /** initialize */
  void
  initialize();

protected:
  /** Stuff on start up */
  void
  firstDataSetup();

  /** Checkout output grids are initialized to current coverage */
  void
  checkOutputGrids(std::shared_ptr<LatLonHeightGrid> input);

  /** Precalculated vil weights */
  float myVilWeights[100];

  /** Current coverage */
  LLCoverageArea myCachedCoverage;

  /** Cache for vilgrid */
  std::shared_ptr<LatLonGrid> myVilGrid;

  /** Cache for vildgrid */
  std::shared_ptr<LatLonGrid> myVildGrid;

  /** Cache for viigrid */
  std::shared_ptr<LatLonGrid> myViiGrid;

  /** Cache for maxgustp */
  std::shared_ptr<LatLonGrid> myMaxGust;

  /** Field key */
  int myHeight263;

  /** Field key */
  int myHeight233;
};

/**
 * Third stage merger/fusion algorithm.
 *
 * This will run algorithms on the 3D cube from stage2
 *
 * @author Robert Toomey
 * @author Valliappa Lakshman
 *
 **/
class RAPIOFusionAlgs : public rapio::RAPIOAlgorithm {
public:

  /** Create tile algorithm */
  RAPIOFusionAlgs(){ };

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

  /** Initialization done on first incoming data */
  void
  firstDataSetup();

protected:
  /** First alg attempt */
  std::vector<std::shared_ptr<ArrayAlgorithm3D> > myAlgs;
};
}
