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
 * We need index ability to reference X,Y,Z values by source
 *
 * I'm gonna abstract this because eh we could put the data into a dynamodb or something
 * later or even a real database if we can get it fast enough.  This first class will
 * just use ram to store things.
 *
 */

/** Ultimate observation stored. Size here is stupid important.
 * Due to conus size, we need to minimize observation handling on incoming data
 * The storage here is required per x,y,z point, so the smaller the better.
 */
class Observation : public Data {
public:
  Observation(short xin, short yin, char zin, float vin, float win, time_t tin) :
    x(xin), y(yin), z(zin), v(vin), w(win), t(tin){ }

  // Reference to XYZ Tree (to delete/modify/merge in XYZ by source)
  // This is 5 bytes.  We 'could' use a size_t i into data, but that's 8 bytes
  short x;
  short y;
  char z;

  // FIXME: We could use a short (2 bytes) and use relative time from the source,
  // but this will require updating the relative times.  Future RAM optimization
  time_t t; // 8 bytes (we could do 2 with relative times)

  // Data we store for merging.  Currently simple weight average
  float v; // 4 bytes
  float w; // 4 bytes
};

/** Store a Source Observation List.  Due to the size of output CONUS we group
 * observations by source to allow quickly updating incoming data. */
class SourceList : public Data {
public:
  /** STL unordered map */
  SourceList(){ }

  SourceList(const std::string& n, short i) : myName(n), myID(i), myTime(0)
  { }

  // FIXME: 'maybe' we inline get/set methods when things get stable

  // Meta data stored for this observation list
  std::string myName;
  short myID; // needed? This will probably have to be IN the obs for fast reverse lookup

  // The time given by the group of 3D points that came from this tilt of data.
  // There can be older non-expired points
  Time myTime;

  // Points stored for the meta data
  std::vector<Observation> myObs;
};

/** (AI) Handle a group of source observation lists that have unique ID keys
 * that can be back-referenced by an x,y,z array.  This allows
 * pulling meta data for observation points during merging if
 * needed. */
template <typename T>
class ObservationManager : public Data {
public:

  /** Given source information, find the already created source observation list,
   * or create a new unique one and return it */
  SourceList&
  getSourceList(const std::string& name)
  {
    // Hunt for source list by name
    for (auto& pair: myObservationMap) { // O(N) with hashing
      if (pair.second.myName == name) {
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
    myObservationMap[newKey] = SourceList(name, newKey);

    // Return the reference
    return myObservationMap.at(newKey);
  }

  /** Replace the reference to source list in the lookup */
  void
  setSourceListRef(const T& key, SourceList& r)
  {
    myObservationMap[key] = r;
  }

  /** Get the raw reference to source list in the lookup */
  SourceList&
  getSourceListRef(const T& key)
  {
    return myObservationMap.at(key);
  }

  /** Remove a source list from our lookup */
  void
  remove(SourceList& r)
  {
    const auto name = r.myName;

    for (auto it = myObservationMap.begin(); it != myObservationMap.end(); ++it) {
      if (it->second.myName == name) {
        myAvailableKeys.push_back(it->second.myID);
        myObservationMap.erase(it);
        break;
      }
    }
    // for (size_t i = 0; i < myAvailableKeys.size(); ++i) {
    //  LogInfo("Keys: " << myAvailableKeys[i] << "\n");
    // }
  }

  /** Iterators for begin access, hiding implementation details */
  typename std::unordered_map<T, SourceList>::iterator
  begin()
  {
    return myObservationMap.begin();
  }

  /** Iterators for end access, hiding implementation details */
  typename std::unordered_map<T, SourceList>::iterator
  end()
  {
    return myObservationMap.end();
  }

protected:

  /** The map of keys to Source Observation Lists */
  std::unordered_map<T, SourceList> myObservationMap;

  /** Old keys we can reuse */
  std::vector<T> myAvailableKeys;

  /** Key number for new ones. */
  short myNextKey = 0;
};

/** Structure stored in the xyz tree.  References data in the source list tree */
class XYZObs : public Data {
public:

  /** Create a XYZ back reference observation into a source */
  XYZObs(short i, size_t e, Observation * at) : myAt(at){ }

  short myID; // ID for which Source storage
  //  size_t myOffset; // Element in the xyz...
  Observation * myAt; // Pointer possibly saves memory but we have to be careful with vector resizes
                      // and 'bleh' if we have to store ID anyway for meta info then memory
                      // will be the same.  If we end up not needing myID then we can save some
                      // One advantage to the direct pointer is possibly speed in the merge loop,
                      // since we don't have to add offset to the id lookup
};

/** Structure stored in the xyz tree.  References a set of XYZObs */
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

  /** Get a source node for a given key */
  SourceList&
  getSourceList(const std::string& name);

  /** Get a new empty source for gathering new observations. */
  SourceList
  getNewSourceList(const std::string& name)
  {
    return SourceList(name, -1);
  }

  /** Add observation to a source list */
  void
  addObservation(SourceList& list, float v, float w, size_t x, size_t y, size_t z, time_t t);

  /** Merge observations from an old source and new source with overlap reduction */
  void
  mergeObservations(SourceList& oldSource, SourceList& newSource, const time_t cutoff);

  /** Add missing mask observation */
  void
  addMissing(SourceList& fromSource, size_t x, size_t y, size_t z, time_t time);

  /** Link back references to observations in the x,y,z tree */
  void
  linkObservations(SourceList& list);

  /** Unlink back references to observations in the x,y,z tree */
  void
  unlinkObservations(SourceList& list);

  /** Clear all observations for a given source list */
  void
  clearObservations(SourceList& list);

  /** Debugging print out each source list and points held */
  void
  dumpSources();

  /** Debugging print out for full XYZ (which is slower) */
  void
  dumpXYZ();

  /** First silly simple weighted merger */
  void
  mergeTo(std::shared_ptr<LLHGridN2D> cache, const time_t cutoff);

  /** Attempt to purge times from database */
  void
  timePurge(Time atTime, TimeDuration interval);

  /** Utility marked points */
  static std::shared_ptr<std::unordered_set<size_t> > myMarked;
protected:
  /** Size in X of entire database */
  size_t myNumX;

  /** Size in Y of entire database */
  size_t myNumY;

  /** Size in Z of entire database */
  size_t myNumZ;

  /** Observation manager handling a list of named sources */
  ObservationManager<short> myObservationManager;

  // FIXME: These grow forever until RAM is swallowed up.
  // We need to allow deleting of empty nodes in the sparse trees here

  /** My xyz back reference list/tree */
  SparseVector<XYZObsList> myXYZs;

  /** My missing mask */
  SparseVector<time_t> myMissings;
};
}
