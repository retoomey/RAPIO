#pragma once

/** RAPIO API */
#include <RAPIO.h>
#include "rLLCoverageArea.h"
#include "rLLHGridN2D.h"
#include "rStage2Data.h"

#include <unordered_map>

// Gives 2^9-1 or 511 source/radar support
#define SOURCE_KEY_BITS 9

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
 *
 * NOTE: Do not make these classes virtual unless you want to explode your RAM
 */
class Observation : public Data {
public:
  /** Create an observation */
  Observation(short xin, short yin, char zin, time_t tin, float rin) :
    x(xin), y(yin), t(tin), r(1000.0 * rin){ } // z(zin), t(tin){ } // , r(rin){ }

  // All observations will have to forward reference the giant x,y,z tree
  // that back-references them.
  // Reference to XYZ Tree (to delete/modify/merge in XYZ by source)
  // This is 5 bytes.  We 'could' use a size_t i into data, but that's 8 bytes
  short x;
  short y;
  //  char z; Pulled out into multi vector messy but saves this memory

  // FIXME: We could use a short (2 bytes) and use relative time from the source,
  // but this will require updating the relative times.  Future RAM optimization
  time_t t; // 8 bytes (we could do 2 with relative times)

  // We store range as meters (2 bytes)  Need this for dynamic nearest comparison
  // Assuming here that meter resolution is high enough
  short r;
};

/** Observation storing a non-missing data value */
class VObservation : public Observation {
public:
  VObservation(short xin, short yin, char zin, float vin, float win, time_t tin, float rin) :
    v(vin), w(win), Observation(xin, yin, zin, tin, rin){ }

  // Data we store for merging.  Currently simple weight average
  float v; // 4 bytes
  float w; // 4 bytes
};

/** Observation storing a missing data value */
class MObservation : public Observation {
public:
  MObservation(short xin, short yin, char zin, time_t tin, float rin) :
    Observation(xin, yin, zin, tin, rin){ }
};

/** Store a Source Observation List.  Due to the size of output CONUS we group
 * observations by source to allow quickly updating incoming data. */
class SourceList : public Data {
public:
  /** STL unordered map */
  SourceList(){ }

  /** Create a source list with a number of levels.
   * FIXME: levels are hardcoded..should be the Z passed in.
   * It's cheap to have 'more' layers since they will just be empty */
  SourceList(const std::string& n, short i, size_t levels = 35) : myName(n), myID(i), myTime(0), myLevels(levels),
    myAObs(levels), myAMObs(levels)
  { }

  /** Add observation to observation list */
  inline void
  addObservation(short x, short y, char z, float v, float w, time_t t, float r)
  {
    // Old stuff (direct Z stored):
    //   myObs.push_back(VObservation(x, y, z, v, w, t, r));
    // New stuff:
    myAObs[z].push_back(VObservation(x, y, z, v, w, t, r));
  }

  /** Add missing to missing list */
  inline void
  addMissing(short x, short y, char z, time_t t, float r)
  {
    // Old stuff (direct Z stored):
    //   myMObs.push_back(MObservation(x, y, z, t, r));
    // New stuff:
    myAMObs[z].push_back(MObservation(x, y, z, t, r));
  }

  /** Clear observations */
  inline void
  clear()
  {
    // Old stuff (direct Z stored):
    //    myObs.clear();
    //    myMObs.clear();
    // New stuff:
    for (size_t i = 0; i < myLevels; ++i) {
      myAObs[i].clear();
      myAMObs[i].clear();
    }
  }

  // FIXME: 'maybe' we inline get/set methods when things get stable

  // Meta data stored for this observation list
  std::string myName;
  unsigned short myID : SOURCE_KEY_BITS; // needed? This will probably have to be IN the obs for fast reverse lookup

  // The late 'main' time of a group of data coming in ingest.
  // Note: Typically the observations stored in us will contain times <= this one.
  Time myTime;

  /** Number of levels we store */
  size_t myLevels;
  // ---------------------------------------------------
  // Stuff we store separately to save memory.
  // Store by a vector of Z (to remove Z storage from observations)
  // and typically Z is small so we can loop here.
  // and seperate by storage type, since values store different
  // stuff then missing values.
  std::vector<std::vector<VObservation> > myAObs;
  std::vector<std::vector<MObservation> > myAMObs;

  // Experimental Z array.  This allows us to save RAM.
  // Values non-missing stored for this source
  //  std::vector<VObservation> myObs;

  // Missing values stored for this source
  //  std::vector<MObservation> myMObs;
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
  // SourceList&
  std::shared_ptr<SourceList>
  getSourceList(const std::string& name)
  {
    // Hunt for source list by name
    for (auto& pair: myObservationMap) { // O(N) with hashing
      if (pair.second->myName == name) {
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
    // myObservationMap[newKey] = SourceList(name, newKey);
    myObservationMap[newKey] = std::make_shared<SourceList>(name, newKey);

    // Return the reference
    return myObservationMap.at(newKey);
  }

  /** Replace the reference to source list in the lookup */
  void
  setSourceList(const T& key, std::shared_ptr<SourceList> r)
  {
    myObservationMap[key] = r;
  }

  #if 0
  /** Replace the reference to source list in the lookup */
  void
  setSourceListRef(const T& key, SourceList& r)
  {
    myObservationMap[key] = r;
  }

  /** Get the raw reference to source list in the lookup */
  // SourceList&
  std::shared_ptr<SourceList>
  getSourceListRef(const T& key)
  {
    return myObservationMap.at(key);
  }

  #endif // if 0

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
  // typename std::unordered_map<T, SourceList>::iterator
  typename std::unordered_map<T, std::shared_ptr<SourceList> >::iterator
  begin()
  {
    return myObservationMap.begin();
  }

  /** Iterators for end access, hiding implementation details */
  // typename std::unordered_map<T, SourceList>::iterator
  typename std::unordered_map<T, std::shared_ptr<SourceList> >::iterator
  end()
  {
    return myObservationMap.end();
  }

protected:

  /** The map of keys to Source Observation Lists */
  // std::unordered_map<T, SourceList> myObservationMap;
  std::unordered_map<T, std::shared_ptr<SourceList> > myObservationMap;

  /** Old keys we can reuse */
  std::vector<T> myAvailableKeys;

  /** Key number for new ones. */
  short myNextKey = 0;
};

/** Structure stored in the xyz tree.  References data in the source list tree */
class XYZObs : public Data {
public:

  /** Create a XYZ back reference observation into a source */
  XYZObs(short i, size_t e, bool missing, Observation * at) : myID(i), isMissing(missing), myAt(at){ }

  /** Update in place */
  inline void
  set(short i, bool missing, Observation * at)
  {
    myID      = i;
    isMissing = missing;
    myAt      = at;
  }

  // Things are byte aligned, so multiple of 8 get stored anyway, but we want to minimize it
  // short myID; // ID for which Source storage
  unsigned short myID : SOURCE_KEY_BITS; // Gives 2^SOURCE_KEY_BITS-1 radars
  bool isMissing : 1;                    // What does our pointer point to? Currently value or missing arrays
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
  /** Size of observation references stored */
  inline size_t size(){ return obs.size(); }

  /** Add raw observation reference stored */
  inline void
  add(short id, size_t s, bool missing, Observation * at)
  {
    for (size_t i = 0; i < obs.size(); ++i) {
      // Found a reusable node
      if (obs[i].myAt == nullptr) {
        obs[i].set(id, missing, at);
        return;
      }
    }
    // Create a new node (slow ram allocation, etc.)
    obs.push_back(XYZObs(id, s, missing, at));
  }

  /** Iterator for begin access, hiding implementation details */
  typename std::vector<XYZObs>::iterator
  begin()
  {
    return obs.begin();
  }

  /** Iterators for end access, hiding implementation details */
  typename std::vector<XYZObs>::iterator
  end()
  {
    return obs.end();
  }

  // protected: to do
  // Possible meta info later for debugging or speedup...
  // Possibly doing custom storage here might make things smaller could
  // be worth doing
  std::vector<XYZObs> obs;
};

/** FusionDatabase maintains the collection of sources which store various types
 * of observations.  It also maintains a X,Y,Z grid that backreferences various
 * observations */
class FusionDatabase : public Data {
public:
  /** The Database is for a 3D cube */
  FusionDatabase(size_t x, size_t y, size_t z) : myNumX(x), myNumY(y), myNumZ(z), myXYZs({ x, y, z })// ,
  // myMissings({ x, y, z })
  { };

  /** Get a source node for a given key */
  // SourceList&
  std::shared_ptr<SourceList>
  getSourceList(const std::string& name);

  /** Get a new empty source for gathering new observations. */
  // SourceList
  std::shared_ptr<SourceList>
  getNewSourceList(const std::string& name)
  {
    // return SourceList(name, -1);
    return std::make_shared<SourceList>(name, -1);
  }

  /** Add observation to a source list */
  void
  addObservation(SourceList& list, float v, float w, float w2, size_t x, size_t y, size_t z, time_t t);

  /** Merge observations from an old source and new source with overlap reduction */
  void
  // mergeObservations(SourceList& oldSource, SourceList& newSource, const time_t cutoff);
  mergeObservations(std::shared_ptr<SourceList> oldSourcePtr, std::shared_ptr<SourceList> newSourcePtr,
    const time_t cutoff);

  /** Add missing mask observation */
  void
  addMissing(SourceList& fromSource, float w, float w2, size_t x, size_t y, size_t z, time_t time);

  /** Link a vector of observations to X,Y,Z grid */
  template <typename T>
  inline void
  linkVector(std::vector<T>& in, unsigned short id, bool missing, size_t z)
  {
    size_t s = 0;

    for (auto& o: in) {
      size_t i = myXYZs.getIndex3D(o.x, o.y, z);
      XYZObsList * node = myXYZs.get(i); // Sparse so could be null

      auto * at = &in[s]; // only good if vector doesn't move

      if (node == nullptr) {
        node = new XYZObsList(); // humm class can auto new I think
        myXYZs.set(i, node);
      }
      node->add(id, s, missing, at);
      // node->obs.push_back(XYZObs(id, s, missing, at));
      s++;
    }
  }

  /** Unlink vector of observations from X,Y,Z grid */
  template <typename T>
  inline void
  unlinkVector(std::vector<T>& in, size_t z)
  {
    auto& have = *myMarked;
    // -----------------------------------------
    // Update xyz tree
    size_t deleted = 0;

    for (auto& o: in) {
      // Find the x,y,z tree node we're in...
      size_t i = myXYZs.getIndex3D(o.x, o.y, z);


      auto node = myXYZs.get(i); // Sparse so could be null
      if (node != nullptr) {
        bool found = false;

        auto& l = node->obs;

        // This code easier to actually understand
        // const size_t s = l.size();
        const size_t s = node->size();
        size_t at      = 0;
        while (at < s) {
          if (l[at].myAt == &o) {
            // ------------------------------
            // We only actually delete nodes that aren't being replaced with new
            // in a bit.  Otherwise we stick nullptr in there temporarily and the link vector will
            // replace it.  We 'could' do in single pass but would require something
            // connecting the random new x,y,z points to the old x,y,z points (move memory)
            if (have.find(i) != have.end()) {
              l[at].myAt = nullptr; // mark available for the link vector function
            } else {
              // ------------------------------
              // Implement swap and pop for deletion
              // This is not being replaced with new values
              //
              // Copy the last in the list into our position...
              l[at] = l[s - 1];
              l.pop_back();
              if (l.size() < 1) {
                myXYZs.clear(i); // Release all memory for empty nodes
                deleted++;
              }
              // ------------------------------
            }
            found = true;
            break; // only have one
          }

          at++;
        }

        if (!found) {
          LogSevere("Missing node, exiting\n");
          exit(1);
        }
        #if 0
        // Find first (we should have only one radar/elev stored per node)
        auto it = std::find_if(l.begin(), l.end(), [&](const XYZObs& obj) {
            const auto& o = in;
            // return obj.myID == list.myID; ID check
            // Pointer in range of myObs? Note myObs is still full right now
            if (o.empty()) { return false; } // If empty can't be in it
            const auto * s = &o.front();
            const auto * e = &o.back() + 1;
            return obj.myAt >= s && obj.myAt < e;
          });
        if (it != l.end()) {
          // Swap the element with the last element and kill it
          // this avoids moving memory around "swap and pop"
          std::iter_swap(it, l.end() - 1);
          l.pop_back();
          if (l.size() < 1) {
            myXYZs.clear(i); // Release all memory for empty nodes
          }
        } else {
          LogSevere("Radar/XYZ tree missing expected error, shouldn't happen so exiting.\n");
          exit(1);
        }
        #endif // if 0
      } else {
        // Maybe warn..I don't think this should happen ever...
        // FIXME: Maybe do some recovery/fixing if this happens to happen,
        // we don't like algorithms just crashing
        LogSevere("Radar/XYZ tree mismatch error, shouldn't happen so exiting.\n");
        exit(1);
      }
    }
    // LogSevere("Deleted oh boy like " << deleted << "\n");
  } // unlinkVector

  /** Link back references to observations in the x,y,z tree */
  //  void
  //  linkObservations(SourceList& list);

  /** Unlink back references to observations in the x,y,z tree */
  //  void
  //  unlinkObservations(SourceList& list);

  /** Clear all observations for a given source list */
  //  void
  //  clearObservations(SourceList& list);

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
  SparseVector2<XYZObsList> myXYZs;

  /** My missing mask */
  // SparseVector<time_t> myMissings;
};
}
