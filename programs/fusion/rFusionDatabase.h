#pragma once

/** RAPIO API */
#include <RAPIO.h>
#include "rLLCoverageArea.h"
#include "rLLHGridN2D.h"
#include "rStage2Data.h"

#include <unordered_map>

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

/** Sparse tree stores nodes in the lookup, vs a single vector.  The single vector has
 * the downsize of being difficult to delete N objects due to the references being related.
 * Sparse tree uses pointers per lookup. */
class SparseTree : public DimensionMapper {
public:

  #if 0

  /** Construct a sparse tree of a given maxSize.  Note it's really maxSize-1 since one key is reserved
   * for missing (the max). */
  SparseTree(size_t maxSize) : DimensionMapper(maxSize),
    myLookupPtr(StaticVector::Create(maxSize, true)), myLookup(*myLookupPtr)
  {
    myLookup.clearAllBits(); // Make 'empty' with 0
    myMissing = nullptr;
  }

  /** Construct a sparse tree of dimensions. */
  SparseTree(std::vector<size_t> dimensions) : DimensionMapper(dimensions),
    myLookupPtr(StaticVector::Create(calculateSize(dimensions), true)), myLookup(*myLookupPtr)
  {
    myLookup.clearAllBits(); // Make 'empty' with 0
    myMissing = nullptr;
  }

  #endif // if 0
};

/** Ultimate observation stored. Size here is stupid important.
 * Due to conus size, we need radar tree to reduce x,y,z grid sampling on incoming tilts
 * The storage here is required per x,y,z point, so the smaller the better.
 * Why no time or radar name?  When merging we always come from the XYZObs,
 * which has an id reference to the RadarObsList which contains all that metadata. */
class RadarObs : public Data {
public:
  RadarObs(short xin, short yin, char zin, float vin, float win) :
    x(xin), y(yin), z(zin), v(vin), w(win){ }

  // Reference to XYZ Tree (to delete/modify in XYZ by Radar)
  // This is 5 bytes.  We 'could' use a size_t i into data, but that's 8 bytes
  short x;
  short y;
  char z; // for moment..can pull up I think

  float v; // 4 bytes
  float w; // 4 bytes
};

/** Store a Radar Observation List.  Due to the size of things we group
 * observations by radar to allow quickly handling incoming data. */
class RadarObsList : public Data {
public:
  /** STL unordered map */
  RadarObsList(){ }

  RadarObsList(const std::string& n, short i, float e) : myName(n), myID(i), myElevDegs(e), myTime(0), myExpired(false)
  { }

  // Meta data stored for this observation list
  std::string myName;
  short myID; // needed? This will probably have to be IN the obs for fast reverse lookup
  float myElevDegs;

  // The time for all points within this tilt of data
  // Well, ok..the 3D points that came from this tilt of data
  Time myTime;

  // Was this cleared by expiration?
  bool myExpired;

  // Points stored for the meta data
  std::vector<RadarObs> myObs;
};

/** (AI) Handle a group of radar obs lists that have unique ID keys
 * that can be back-referenced by the observation.  This allows
 * pulling meta data for observation points during merging if
 * needed. */
template <typename T>
class RadarObsManager : public Data {
public:

  /** Given radar information, find the already created radar observation list,
   * or create a new unique one and return it */
  RadarObsList&
  getRadar(const std::string& radarname, float elevDegs)
  {
    // Hunt for radarname/elevDegs
    for (auto& pair: myRadarObsMap) { // O(N) with hashing
      if ((pair.second.myName == radarname) &&
        (pair.second.myElevDegs == elevDegs))
      {
        return pair.second;
      }
    }

    // Not found, so we create one and give it the next available key
    T newKey;

    if (myAvailableKeys.empty()) {
      // No available keys, generate a new one after the current max key
      newKey = myNextKey++;
    } else {
      // Reuse an available key.  Shifting vector to keep things kinda ordered
      newKey = *myAvailableKeys.begin();
      myAvailableKeys.erase(myAvailableKeys.begin());
    }
    myRadarObsMap[newKey] = RadarObsList(radarname, newKey, elevDegs);

    // Return the reference
    return myRadarObsMap.at(newKey);
  }

  /** Remove a radar/elevation from our list */
  void
  remove(RadarObsList& r)
  {
    const auto radarname = r.myName;
    const auto elevDegs  = r.myElevDegs;

    // Hunt for radarname/elevDegs
    // for(auto& pair: myRadarObsMap){ // O(N) with hashing
    for (auto it = myRadarObsMap.begin(); it != myRadarObsMap.end(); ++it) {
      if ((it->second.myName == radarname) &&
        (it->second.myElevDegs == elevDegs))
      {
        myAvailableKeys.push_back(it->second.myID);
        myRadarObsMap.erase(it);
        break;
      }
    }
  }

  /** Iterators for begin access, hiding implementation details */
  typename std::unordered_map<T, RadarObsList>::iterator
  begin()
  {
    return myRadarObsMap.begin();
  }

  /** Iterators for end access, hiding implementation details */
  typename std::unordered_map<T, RadarObsList>::iterator
  end()
  {
    return myRadarObsMap.end();
  }

protected:

  /** The map of keys to Radar Observation Lists */
  std::unordered_map<T, RadarObsList> myRadarObsMap;

  /** Old keys we can reuse */
  std::vector<T> myAvailableKeys;

  /** Key number for new ones. */
  short myNextKey = 0;
};

/** Structure stored in the xyz tree.  References data in the radar tree */
class XYZObs : public Data {
public:
  // STL
  //   XYZObs();

  // overflow
  // XYZObs(short i, short e) : myID(i), myOffset(e){ }
  // XYZObs(short i, size_t e, RadarObs* at) : myID(i), myOffset(e){ }
  XYZObs(short i, size_t e, RadarObs * at) : myAt(at){ }

  // For 'find' operation when purging
  // bool operator==(const XYZObs& other) const {
  //   return ((this->myID == other.myID) && (this->myOffset == other.myOffset));
  // }

  // RadarObs* obs;  Vector resizes can move pointers to elements around..
  short myID; // ID for which Radar storage
  //  size_t myOffset; // Element in the xyz...
  RadarObs * myAt; // Pointer possibly saves memory but we have to be careful with vector resizes
                   // and 'bleh' if we have to store ID anyway for meta info then memory
                   // will be the same.  If we end up not needing myID then we can save some
                   // One advantage to the direct pointer is possibly speed in the merge loop,
                   // since we don't have to add offset to the id lookup
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
  FusionDatabase(size_t x, size_t y, size_t z) : myNumX(x), myNumY(y), myNumZ(z), myXYZs({ x, y, z }),
    myMissings({ x, y, z })
  { };

  /** Get a radar node for a given key */
  RadarObsList&
  getRadar(const std::string& radarname, float elevDegs);

  /** Add observation to a radar node */
  void
  addObservation(RadarObsList& list, float v, float w, size_t x, size_t y, size_t z);

  /** Add missing mask observation */
  void
  addMissing(size_t x, size_t y, size_t z, time_t time);

  /** Clear all observations for a given radar node */
  void
  clearObservations(RadarObsList& list);

  /** Reserve any memory for incoming data */
  void
  reserveSizes(RadarObsList& list, size_t values, size_t missings);

  /** Debugging print out each radar/elev key and points held */
  void
  dumpRadars();

  /** Debugging print out for full XYZ (which is slower) */
  void
  dumpXYZ();

  /** First silly simple weighted merger */
  void
  mergeTo(std::shared_ptr<LLHGridN2D> cache);

  /** Attempt to purge times from database */
  void
  timePurge(Time atTime, TimeDuration interval);

protected:
  /** Size in X of entire database */
  size_t myNumX;

  /** Size in Y of entire database */
  size_t myNumY;

  /** Size in Z of entire database */
  size_t myNumZ;

  /** Radar obs manager handling radar tree */
  RadarObsManager<short> myRadarObsManager;

  /** My xyz back reference list/tree */
  SparseVector<XYZObsList> myXYZs;

  /** My missing mask */
  SparseVector<time_t> myMissings;
};
}
