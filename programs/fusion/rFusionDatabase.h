#pragma once
#include "rLLCoverageArea.h"
#include "rLLHGridN2D.h"
#include "rStage2Data.h"
#include "rWindBinaryTable.h"
#include <array>
#include <stdexcept>
#include <unordered_map>

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

class Observation {
public:
  Observation(short xin, short yin, char zin, time_t tin) : x(xin), y(yin), t(tin) {}
  short x;
  short y;
  time_t t;
};

template <size_t N>
class GenericObservation : public Observation {
public:
  GenericObservation(short xin, short yin, char zin, const std::array<float, N>& datain, time_t tin)
    : Observation(xin, yin, zin, tin), data(datain) {}
  std::array<float, N> data;
};

class MObservation : public Observation {
public:
  MObservation(short xin, short yin, char zin, time_t tin) : Observation(xin, yin, zin, tin) {}
};

class SourceListBase {
public:
  SourceListBase(const std::string& n, short i) : myName(n), myID(i), myTime(0) {}
  virtual ~SourceListBase() = default;

  virtual void timePurge(time_t cutoff) = 0;
  virtual void unionMerge(SourceListBase& newSource, Bitset1& mask, time_t cutoff, size_t& timePurged, size_t& restored) = 0;
  virtual void addMissing(short x, short y, char z, time_t t) = 0;

  std::string myName;
  unsigned short myID : SOURCE_KEY_BITS;
  Time myTime;
};

template <size_t N>
class PayloadSourceList : public SourceListBase {
public:
  PayloadSourceList(const std::string& n, short i, size_t levels = 35)
    : SourceListBase(n, i), myLevels(levels), myObs(levels), myAMObs(levels) {}

  inline void addObservation(short x, short y, char z, const std::array<float, N>& data, time_t t) {
    myObs[z].push_back(GenericObservation<N>(x, y, z, data, t));
  }

  virtual void addMissing(short x, short y, char z, time_t t) override {
    myAMObs[z].push_back(MObservation(x, y, z, t));
  }

  inline void clear() {
    for (size_t i = 0; i < myLevels; ++i) {
      myObs[i].clear();
      myAMObs[i].clear();
    }
  }

  template <typename T>
  inline void timePurgeV(std::vector<T>& v, time_t cutoff) {
    size_t i = 0;
    while (i < v.size()) {
      if (v[i].t < cutoff) {
        v[i] = v.back();
        v.pop_back();
      } else {
        ++i;
      }
    }
  }

  virtual void timePurge(time_t cutoff) override {
    for (size_t z = 0; z < myLevels; ++z) {
      timePurgeV(myObs[z], cutoff);
      timePurgeV(myAMObs[z], cutoff);
    }
  }

  template <typename T>
  inline void unionMergeV(Bitset1& mask, size_t z, std::vector<T>& new1, std::vector<T>& old1, time_t cutoff, size_t& timePurged, size_t& restored) {
    for (auto& o: old1) {
      if (!mask.get13D(o.x, o.y, z)) {
        if (o.t < cutoff) {
          timePurged++;
        } else {
          new1.push_back(o);
          restored++;
        }
      }
    }
  }

  virtual void unionMerge(SourceListBase& newSourceBase, Bitset1& mask, time_t cutoff, size_t& timePurged, size_t& restored) override {
    auto& newSource = static_cast<PayloadSourceList<N>&>(newSourceBase);
    for (size_t z = 0; z < myLevels; ++z) {
      unionMergeV(mask, z, newSource.myObs[z], myObs[z], cutoff, timePurged, restored);
      unionMergeV(mask, z, newSource.myAMObs[z], myAMObs[z], cutoff, timePurged, restored);
    }
  }

  size_t myLevels;
  std::vector<std::vector<GenericObservation<N>>> myObs;
  std::vector<std::vector<MObservation>> myAMObs;
};

template <typename T>
class ObservationManager {
public:
  size_t getPayloadSize() const { return myLockedPayloadSize; }

  bool lockPayloadSize(size_t incomingSize) {
    if (myLockedPayloadSize == 0) {
      myLockedPayloadSize = incomingSize;
      return true;
    }
    return myLockedPayloadSize == incomingSize;
  }

  std::shared_ptr<SourceListBase> getSourceList(const std::string& name, size_t numZ) {
    for (auto& pair: myObservationMap) {
      if (pair.second->myName == name) {
        return pair.second;
      }
    }
    T newKey;
    if (myAvailableKeys.empty()) {
      newKey = myNextKey++;
    } else {
      newKey = *myAvailableKeys.begin();
      myAvailableKeys.erase(myAvailableKeys.begin());
    }

    std::shared_ptr<SourceListBase> newList;
    if (myLockedPayloadSize == 5) {
        newList = std::make_shared<PayloadSourceList<5>>(name, newKey, numZ);
    } else {
        newList = std::make_shared<PayloadSourceList<2>>(name, newKey, numZ);
    }

    myObservationMap[newKey] = newList;
    return newList;
  }

  void setSourceList(const T& key, std::shared_ptr<SourceListBase> r) {
    myObservationMap[key] = r;
  }

  void remove(SourceListBase& r) {
    const auto name = r.myName;
    for (auto it = myObservationMap.begin(); it != myObservationMap.end(); ++it) {
      if (it->second->myName == name) {
        myAvailableKeys.push_back(it->second->myID);
        myObservationMap.erase(it);
        break;
      }
    }
  }

  typename std::unordered_map<T, std::shared_ptr<SourceListBase>>::iterator begin() { return myObservationMap.begin(); }
  typename std::unordered_map<T, std::shared_ptr<SourceListBase>>::iterator end() { return myObservationMap.end(); }

protected:
  std::unordered_map<T, std::shared_ptr<SourceListBase>> myObservationMap;
  std::vector<T> myAvailableKeys;
  short myNextKey = 0;
  size_t myLockedPayloadSize = 0;
};

class FusionDatabase {
public:
  FusionDatabase(size_t x, size_t y, size_t z) : myNumX(x), myNumY(y), myNumZ(z), myXYZs({ x, y, z }), myHaves({ x, y, z }), myMissings(x * y * z) {
    for (size_t i = 0; i < myMissings.size(); ++i) {
      myMissings[i] = std::numeric_limits<time_t>::min();
    }
  };

  void ingestNewData(std::shared_ptr<BinaryTable> data, time_t cutoff, size_t& missingcounter, size_t& points, size_t& total);
  
  std::shared_ptr<SourceListBase> getSourceList(const std::string& name) {
    return myObservationManager.getSourceList(name, myNumZ);
  }
  
  std::shared_ptr<SourceListBase> getNewSourceList(const std::string& name) {
    if (myObservationManager.getPayloadSize() == 5) return std::make_shared<PayloadSourceList<5>>(name, -1, myNumZ);
    return std::make_shared<PayloadSourceList<2>>(name, -1, myNumZ);
  }

  void addMissing(SourceListBase& fromSource, size_t x, size_t y, size_t z, time_t time, bool dataNoMissingSet);
  void mergeObservations(std::shared_ptr<SourceListBase> oldSourcePtr, std::shared_ptr<SourceListBase> newSourcePtr, const time_t cutoff);
  void dumpSources();
  void timePurge(Time atTime, TimeDuration interval);

  ObservationManager<short>& getObservationManager() { return myObservationManager; }
  const Bitset1& getHaves() const { return myHaves; }
  const std::vector<time_t>& getMissings() const { return myMissings; }

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
