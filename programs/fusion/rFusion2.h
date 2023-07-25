#pragma once

/** RAPIO API */
#include <RAPIO.h>
#include "rLLCoverageArea.h"
#include "rLLHGridN2D.h"
#include "rStage2Data.h"

namespace rapio {
/** Class for storage of the point cloud.
 * This is for a single moment.
 *
 * We need a database storing N observations at each X,Y,Z.
 * We need to be able to add a new X,Y,Z value sparsely.
 * We need to be able to delete from X,Y,Z based on time expiration (in observation)
 * We need to be able to iterate over X,Y,Z and get the observations quickly per cell
 * We need index ability to reference X,Y,Z values by radar
 *
 * I'm gonna abstract this because eh we could put the data into a dynamodb or something
 * later or even a real database if we can get it fast enough.  This first class will
 * just use ram to store things.
 *
 * In merger we stored by radar/elev to quickly dump off old values.  I 'think' we can
 * brute force our x,y,z tree and avoid having to store x,y,z values in the radar/elev lists.
 * This would drop ram at the cost of cpu, so we'll see how this scales.
 * In other words, if we only store an x,y,z array...we have to iterate over it for any
 * action..and the grid can be large.
 */

/** Ultimate observation stored. Size here is stupid important.
 * Due to conus size, we need radar tree to reduce x,y,z grid sampling on incoming tilts
 * The storage here is required per x,y,z point, so the smaller the better. */
class RadarObs : public Data {
public:
  RadarObs(short xin, short yin, char zin, float vin, float win) :
    x(xin), y(yin), z(zin), v(vin), w(win){ }

  // short id;  // meta information reference
  // time: Some sort of time storage right?
  short x;
  short y;
  char z; // for moment..can pull up I think
  float v;
  float w;
};

class RadarObsList : public Data {
public:
  RadarObsList(const std::string& n, short i, float e) : myName(n), myID(i), myElevDegs(e)
  { }

  // Meta data stored for this observation list
  std::string myName;
  short myID; // needed? This will probably have to be IN the obs for fast reverse lookup
  float myElevDegs;

  // Points stored for the meta data
  std::vector<RadarObs> myObs;
};

/** Structure stored in the xyz tree.  References data in the radar tree */
class XYZObs : public Data {
public:
  // STL
  //   XYZObs();

  XYZObs(short i, short e) : myID(i), myOffset(e){ }

  // RadarObs* obs;  Vector resizes can move pointers to elements around..
  short myID;     // ID for which Radar storage
  short myOffset; // Element in the xyz...
};

/** Structure stored in the xyz tree.  References data in the radar tree */
class XYZObsList : public Data {
public:
  // Possible meta info later for debugging or speedup...
  // Possibly doing custom storage here might make things smaller could
  // be worth doing
  std::vector<XYZObs> obs;
};

class FusionDatabase : public Data {
public:
  /** The Database is for a 3D cube */
  FusionDatabase(size_t x, size_t y, size_t z) : myNumX(x), myNumY(y), myNumZ(z), myXYZs({ x, y, z })
  { };

  /** Get a radar node for a given key */
  RadarObsList&
  getRadar(const std::string& radarname, float elevDegs)
  {
    // Get key ID for observation back referencing (if we even need this);
    // I'm pretty sure at some point observations will have to back reference the source radar info,
    // this key will allow a metadata access
    // Radar/Elev/Z would be nice...
    std::stringstream keymaker;

    keymaker << radarname << elevDegs; // create unique key
    std::string theKey = keymaker.str();

    for (size_t i = 0; i < myKeys.size(); ++i) { // O(N) but only on new file once
      if (myKeys[i] == theKey) {
        return (myRadars[i]);
      }
    }
    auto newID = myKeys.size();

    myRadars.push_back(RadarObsList(radarname, newID, elevDegs));
    myKeys.push_back(theKey);
    return (myRadars[newID]);
  }

  /** Add observation to a radar node */
  void
  addObservation(RadarObsList& list, float v, float w, size_t x, size_t y, size_t z)
  {
    // Add observation to the radar list (<< small)
    // Could speed up length pointer calculations by external
    // FIXME: could/probably should preallocate list size
    size_t s = list.myObs.size();

    // -----------------------------------------
    // Update radar tree
    //
    list.myObs.push_back(RadarObs(x, y, z, v, w));

    // -----------------------------------------
    // Update xyz tree
    //
    // This is slow for entire x,y,z.  Since radar will be much smaller
    // in theory it will be fast enough
    #if 0
    size_t i = myXYZs.getIndex3D(x, y, z);
    XYZObsList * node = myXYZs.get(i); // Sparse so could be null

    if (node == nullptr) {
      // FIXME: sparse could have a set(i) with a default, might be quicker
      // or we could have a getForce method or something that forces create
      XYZObsList newlist;
      node = myXYZs.set(i, newlist); // bleh means sparse vector always grows, never shrinks.  Need to handle that
      // though to be fair I don't think w2merger shrinks either
      if (node != nullptr) {
        node->obs.push_back(XYZObs(list.myID, s));
      }
    } else {
      node->obs.push_back(XYZObs(list.myID, s));
    }
    #endif // if 0
  } // addObservation

  /** Clear all observations for a give radar node */
  void
  clearObservations(RadarObsList& list)
  {
    // FIXME: First loop and update x,y,z list right?
    // Still working on xyz tree
    #if 0
    for (auto& o: list.myObs) {
      // Find the x,y,z tree node we're in...
      size_t i  = myXYZs.getIndex3D(o.x, o.y, o.z);
      auto node = myXYZs.get(i); // Sparse so could be null
      if (node != nullptr) {
        auto& l = node->obs;
        auto it = std::find(l.begin(), l.end(), &o);
        if (it != l.end()) {
          // Swap the element with the last element and kill it
          std::iter_swap(it, l.end() - 1);
          l.pop_back();
        }
      } else {
        // Maybe warn..I don't think this should happen ever...
      }
    }
    #endif // if 0

    // ...then clear the radar list
    list.myObs.clear();
  }

  /** Debugging print out each radar/elev key and points held */
  void
  dumpRadars()
  {
    size_t counter = 0;

    for (size_t i = 0; i < myKeys.size(); ++i) { // O(N) but only on new file once
      auto &r = myRadars[i];
      LogInfo(
        i << ": (" << r.myID << ") " << r.myName << " " << r.myElevDegs << "  is storing " << r.myObs.size() <<
          " observations.\n");
      counter += r.myObs.size();
    }
    LogInfo("Total observations: " << counter << "\n");
  }

protected:
  /** Size in X of entire database */
  size_t myNumX;

  /** Size in Y of entire database */
  size_t myNumY;

  /** Size in Z of entire database */
  size_t myNumZ;

  /** Keys used to reference radars */
  std::vector<std::string> myKeys;

  /** My radar list/tree */
  std::vector<RadarObsList> myRadars;

  /** My xyz back reference list/tree */
  SparseVector<XYZObsList> myXYZs;
};

/**
 * Second stage merger/fusion algorithm.  Gathers input from single stage radar
 * processes for the intention of merging data into a final output.
 * For each tilt, a subset of R radars from a larger set of N radars is used,
 * where as the number of tiles increase, R drops due to geographic coverage.
 * Thus we have R subset of or equal to T.
 * We should horizontally scale by breaking into subtiles that cover the larger
 * merger tile.
 * Some goals:
 * 1.  Automatically tile based on given tiling sizes, vs merger using hard coded
 *     lat/lon values.
 * 2.  Attempt to time synchronize to avoid discontinuities along tile edges.
 *     This will be important to allow more than the 4 hardcoded tiles we use
 *     in the current w2merger.
 * 3.  Possibly send data using web/REST vs the raw/fml notification.
 *
 * @author Robert Toomey
 * @author Valliappa Lakshman
 *
 **/
class RAPIOFusionTwoAlg : public rapio::RAPIOAlgorithm {
public:

  /** Create tile algorithm */
  RAPIOFusionTwoAlg(){ };

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
  firstDataSetup(std::shared_ptr<Stage2Data> d);

  /** Create the cache of interpolated layers */
  void
  createLLGCache(
    const std::string   & outputName,
    const std::string   & outputUnits,
    const LLCoverageArea& g);

  /** Coordinates for the total merger grid */
  LLCoverageArea myFullGrid;

  /** The typename we use for writing cappis */
  std::string myWriteCAPPIName;

  /** The units we use for all output products */
  std::string myWriteOutputUnits;

  /** Cached set of LatLonGrids */
  std::shared_ptr<LLHGridN2D> myLLGCache;

  /** Typename (moment) linked to */
  std::string myTypeName;

  /** Write lat lon grids to output when ingesting a file */
  bool myWriteLLG;

  /** Throttle skip counter to avoid IO spamming during testing.
  * The good news is we're too fast for the current pipeline. */
  size_t myThrottleCount;

  /** My database of 3D point observations */
  std::shared_ptr<FusionDatabase> myDatabase;
};
}
