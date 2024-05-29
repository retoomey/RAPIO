#pragma once

/** RAPIO API */
#include <RAPIO.h>
#include "rLLCoverageArea.h"
#include "rLLHGridN2D.h"
#include "rStage2Data.h"

// Gives 2^9-1 or 511 source/radar support
#define SOURCE_KEY_BITS 9

namespace rapio {
/** Class for storage of the point cloud.
 * This is for a single moment.
 *
 * We need a database storing N observations at each X,Y,Z.
 * We need to be able to delete from X,Y,Z based on time expiration (in observation)
 * We need to be able to iterate over X,Y,Z and get the observations quickly per cell
 *   --Well actually, we just need to hit all the X,Y,Z, but don't have to be in order
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
  Observation(short xin, short yin, char zin, time_t tin) :
    x(xin), y(yin), t(tin){ }

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

  // Ok we're gonna trust FusionRoster completely on the ranges.  If we do any
  // trimming of the list of values per point we'll use time I think.
  //
  // Example: Radars A,B,C. Radar A goes down. We still have 3 radars for the point..haven't expired yet.
  // Roster turns on radar D. We now get radar D. Now we have 4 radars worth of data since A hasn't 'quite' expired yet.
  // So we actuallly merge 4 values instead of the 3 we set we wanted. A does expire out though so things 'fix' themselves.
  // It's a temp issue of having more data than expected if radars toggle status a lot.
  // However the benefit of no range is allowing faster merge and less IO sending stage2 data.
  // Note: if we get more values and we can store we can always pick the 'newest' 3 vs range in this case
  // short r;
};

/** Observation storing a non-missing data value */
class VObservation : public Observation {
public:
  VObservation(short xin, short yin, char zin, float vin, float win, time_t tin) :
    v(vin), w(win), Observation(xin, yin, zin, tin){ }

  // Data we store for merging.  Currently simple weight average
  float v; // 4 bytes
  float w; // 4 bytes
};

/** Observation storing a missing data value */
class MObservation : public Observation {
public:
  MObservation(short xin, short yin, char zin, time_t tin) :
    Observation(xin, yin, zin, tin){ }
};

/** Store a Source Observation List.  Due to the size of output CONUS we group
 * observations by source to allow quickly updating incoming data. */
class SourceList : public Data {
public:
  /** STL unordered map */
  SourceList(){ }

  /** Create a source list with a number of levels. **/
  SourceList(const std::string& n, short i, size_t levels = 35) : myName(n), myID(i), myTime(0), myLevels(levels),
    myAObs(levels), myAMObs(levels)
  { }

  /** Add observation to observation list */
  inline void
  addObservation(short x, short y, char z, float v, float w, time_t t)
  {
    myAObs[z].push_back(VObservation(x, y, z, v, w, t));
  }

  /** Add missing to missing list */
  inline void
  addMissing(short x, short y, char z, time_t t)
  {
    myAMObs[z].push_back(MObservation(x, y, z, t));
  }

  /** Clear observations */
  inline void
  clear()
  {
    for (size_t i = 0; i < myLevels; ++i) {
      myAObs[i].clear();
      myAMObs[i].clear();
    }
  }

  /** General vector time purge. We delete by swapping to the end of
   * vector and popping. */
  template <typename T>
  inline void
  timePurgeV(std::vector<T>& v, time_t cutoff)
  {
    size_t i = 0;

    while (i < v.size()) {
      if (v[i].t < cutoff) { // Our epoch less than cutoff epoch
        // Swap and pop item
        v[i] = v.back();
        v.pop_back();
      } else {
        ++i;
      }
    }
  }

  /** Purge time cutoff */
  inline void
  timePurge(time_t cutoff)
  {
    for (size_t z = 0; z < myLevels; ++z) {
      timePurgeV(myAObs[z], cutoff);
      timePurgeV(myAMObs[z], cutoff);
    }
  }

  /** Add points to a new source not marked in mask and still time valid */
  template <typename T>
  inline void
  unionMergeV(Bitset1& mask, size_t z, std::vector<T>& new1, std::vector<T>& old1, time_t cutoff, size_t& timePurged,
    size_t& restored)
  {
    for (auto& o: old1) {
      if (!mask.get13D(o.x, o.y, z)) {
        if (o.t < cutoff) { // We could wait until global time purge?
          timePurged++;
        } else {
          new1.push_back(o);
          // Do we need to update mask...think we don't have to
          // mask.set13D(o.x, o.y, z);
          restored++;
        }
      }
    }
  }

  /** Add our points to a new source not marked in mask and still time valid. Marked
   * is used to avoid duplicates and properly union the sets. */
  inline void
  unionMerge(SourceList& newSource, Bitset1& mask, time_t cutoff, size_t& timePurged, size_t& restored)
  {
    for (size_t z = 0; z < myLevels; ++z) {
      unionMergeV(mask, z, newSource.myAObs[z], myAObs[z], cutoff, timePurged, restored);
      unionMergeV(mask, z, newSource.myAMObs[z], myAMObs[z], cutoff, timePurged, restored);
    }
  }

  // FIXME: 'maybe' we inline get/set methods when things get stable

  // Meta data stored for this observation list
  std::string myName;

  // deprecated.  Think we can do everything at stage1
  unsigned short myID : SOURCE_KEY_BITS; // needed? This will probably have to be IN the obs for fast reverse lookup

  // The late 'main' time of a group of data coming in ingest.
  // Note: Typically the observations stored in us will contain times <= this one.
  Time myTime;

  /** Number of levels we store */
  size_t myLevels;

  /** Vector of value observations */
  std::vector<std::vector<VObservation> > myAObs;

  /** Vector of missing observations */
  std::vector<std::vector<MObservation> > myAMObs;
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
  typename std::unordered_map<T, std::shared_ptr<SourceList> >::iterator
  begin()
  {
    return myObservationMap.begin();
  }

  /** Iterators for end access, hiding implementation details */
  typename std::unordered_map<T, std::shared_ptr<SourceList> >::iterator
  end()
  {
    return myObservationMap.end();
  }

protected:

  /** The map of keys to Source Observation Lists */
  std::unordered_map<T, std::shared_ptr<SourceList> > myObservationMap;

  /** Old keys we can reuse */
  std::vector<T> myAvailableKeys;

  /** Key number for new ones. */
  short myNextKey = 0;
};

/** FusionDatabase maintains the collection of sources which store various types
 * of observations.  It also maintains a X,Y,Z grid that backreferences various
 * observations */
class FusionDatabase : public Data {
public:
  /** The Database is for a 3D cube */
  FusionDatabase(size_t x, size_t y, size_t z) : myNumX(x), myNumY(y), myNumZ(z), myXYZs({ x, y, z }), myHaves({ x, y,
                                                                                                                 z }),
    myMissings(x * y * z)
  {
    for (size_t i = 0; i < myMissings.size(); ++i) {
      myMissings[i] = std::numeric_limits<time_t>::min();
    }
  };

  /** Ingest new stage2 data */
  void
  ingestNewData(Stage2Data& data, time_t cutoff, size_t& missingcounter, size_t& points, size_t& total);

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
  addObservation(SourceList& list, float v, float w, size_t x, size_t y, size_t z, time_t t);

  /** Merge observations from an old source and new source with overlap reduction */
  void
  // mergeObservations(SourceList& oldSource, SourceList& newSource, const time_t cutoff);
  mergeObservations(std::shared_ptr<SourceList> oldSourcePtr, std::shared_ptr<SourceList> newSourcePtr,
    const time_t cutoff);

  /** Add missing mask observation */
  void
  addMissing(SourceList& fromSource, size_t x, size_t y, size_t z, time_t time);

  /** Debugging print out each source list and points held */
  void
  dumpSources();

  /** First silly simple weighted merger */
  void
  mergeTo(std::shared_ptr<LLHGridN2D> cache, const time_t cutoff, size_t offsetX = 0, size_t offsetY = 0);

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

  /** Observation manager handling a list of named sources */
  ObservationManager<short> myObservationManager;

  /** My xyz back reference list/tree */
  DimensionMapper myXYZs;

  /** My have marked array (bits) */
  Bitset1 myHaves;

  /** My latest missing array mask */
  std::vector<time_t> myMissings;
};
}
