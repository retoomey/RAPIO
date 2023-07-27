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

  // Reference to xyz location so we can be purged
  // Another way?  Can't use pointers, vectors move around
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

  // overflow
  // XYZObs(short i, short e) : myID(i), myOffset(e){ }
  XYZObs(short i, size_t e) : myID(i), myOffset(e){ }

  // For 'find' operation when purging
  // bool operator==(const XYZObs& other) const {
  //   return ((this->myID == other.myID) && (this->myOffset == other.myOffset));
  // }

  // RadarObs* obs;  Vector resizes can move pointers to elements around..
  short myID;      // ID for which Radar storage
  size_t myOffset; // Element in the xyz...
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
  getRadar(const std::string& radarname, float elevDegs);
  #if 0
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
  #endif // if 0

  /** Add observation to a radar node */
  void
  addObservation(RadarObsList& list, float v, float w, size_t x, size_t y, size_t z);

  #if 0
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
  } // addObservation
  #endif // if 0

  /** Clear all observations for a given radar node */
  void
  clearObservations(RadarObsList& list);
  #if 0
  {
    // -----------------------------------------
    // Update xyz tree
    for (auto& o: list.myObs) {
      // Find the x,y,z tree node we're in...
      size_t i  = myXYZs.getIndex3D(o.x, o.y, o.z);
      auto node = myXYZs.get(i); // Sparse so could be null
      if (node != nullptr) {
        auto& l = node->obs;
        // Find first (we should have only one radar/elev stored per node)
        auto it = std::find_if(l.begin(), l.end(), [&](const XYZObs& obj) {
            return obj.myID == list.myID;
          });
        if (it != l.end()) {
          // Swap the element with the last element and kill it
          // this avoids moving memory around "swap and pop"
          std::iter_swap(it, l.end() - 1);
          l.pop_back();
        }
      } else {
        // Maybe warn..I don't think this should happen ever...
        // FIXME: Maybe do some recovery/fixing if this happens to happen,
        // we don't like algorithms just crashing
        LogSevere("Radar/XYZ tree mismatch error, shouldn't happen so exiting.\n");
        exit(1);
      }
    }

    // -----------------------------------------
    // Update radar tree
    // ...then clear the radar list
    list.myObs.clear();
  }
  #endif // if 0

  /** Debugging print out each radar/elev key and points held */
  void
  dumpRadars();
  #if 0
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
  #endif // if 0

  /** Debugging print out for full XYZ (which is slower) */
  void
  dumpXYZ();
  #if 0
  {
    ProcessTimer test("Scanning XYZ tree (long)\n");
    size_t counter = 0;
    for (size_t x = 0; x < myNumX; x++) {
      for (size_t y = 0; y < myNumY; y++) {
        for (size_t z = 0; z < myNumZ; z++) {
          size_t i  = myXYZs.getIndex3D(x, y, z);
          auto node = myXYZs.get(i); // Sparse so could be null
          if (node != nullptr) {
            counter += node->obs.size();
          }
        }
      }
    }
    LogInfo("Total XYZ scanned count is " << counter << "\n");
    LogInfo(test << "\n");
  }
  #endif // if 0

  /** First silly simple weighted merger */
  void
  mergeTo(std::shared_ptr<LLHGridN2D> cache);
  #if 0
  {
    static bool firstTime = true;
    size_t failed         = 0;
    size_t failed2        = 0;

    ProcessTimer test("Merging XYZ tree\n");
    LogSevere("Warning, showstopper detected.  Should probably eat lunch.\n");
    for (size_t z = 0; z < myNumZ; z++) {
      // auto output = cache->get(z);
      std::shared_ptr<LatLonGrid> output = cache->get(z);

      if (output == nullptr) {
        LogSevere("Failed to get layer " << z << " from LLH cache\n");
        return;
      }

      auto& gridtest = output->getFloat2DRef();
      for (size_t x = 0; x < myNumX; x++) { // x currently LON for stage2 right..so xy swapped
        for (size_t y = 0; y < myNumY; y++) {
          size_t i  = myXYZs.getIndex3D(x, y, z);
          auto node = myXYZs.get(i); // Sparse so could be null

          // Silly simple weighted average of everything we got
          float weightedSum = 0.0;
          float totalWeight = 0.0;
          float v = Constants::DataUnavailable; // mask

          if (node != nullptr) {
            bool haveValues = false;
            for (const auto& n: node->obs) {
              if (n.myID < myRadars.size()) { } else {
                failed2 += 1;
                continue;
              }
              const auto &r = myRadars[n.myID];

              if (n.myOffset < r.myObs.size()) { } else {
                static size_t analyze = 0;
                // Let's check things out here...
                if (analyze++ < 20) {
                  LogSevere(" ---> " << n.myOffset << " and size of " << r.myObs.size() << "\n");
                }

                failed += 1;
                continue;
              }
              const auto &data = r.myObs[n.myOffset];

              if (data.v != Constants::MissingData) {
                weightedSum += data.v * data.w;
                totalWeight += data.w;
                haveValues   = true;
              } else {
                v = Constants::MissingData; // mask
              }
            }

            // Final v
            if (haveValues && (totalWeight > 0.001)) {
              v = weightedSum / totalWeight;
            }
          }

          gridtest[y][x] = v;
        }
      }
    }
    if ((failed > 0) || (failed2 > 0)) {
      LogSevere("Woo whoo! Failed hits: " << failed << " and " << failed2 << "\n");
    }
    firstTime = false;
  }
  #endif // if 0

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
}
