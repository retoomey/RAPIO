#include "rFusionDatabase.h"

using namespace rapio;

// ---------------------------------------
// First merge
// No time expiration so long running will get a bit messy,
// but baby steps..time will be next checkin
//
void
FusionDatabase::mergeTo(std::shared_ptr<LLHGridN2D> cache)
{
  static bool firstTime = true;
  size_t failed         = 0;
  size_t failed2        = 0;

  ProcessTimer test("Merging XYZ tree\n");

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
            // For moment during alpha check sizes of things...
            if (n.myID >= myRadars.size()) {
              failed2 += 1;
              continue;
            }
            const auto &r = myRadars[n.myID];

            // For moment during alpha check sizes of things...
            if (n.myOffset >= r.myObs.size()) {
              static size_t analyze = 0;
              // Let's check things out here...
              if (analyze++ < 20) {
                LogInfo(" ---> " << n.myOffset << " and size of " << r.myObs.size() << "\n");
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
    LogSevere("Failed hits: " << failed << " and " << failed2 << "\n");
  }
  firstTime = false;
} // FusionDatabase::mergeTo

RadarObsList&
FusionDatabase::getRadar(const std::string& radarname, float elevDegs)
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

void
FusionDatabase::addObservation(RadarObsList& list, float v, float w, size_t x, size_t y, size_t z)
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
}

void
FusionDatabase::clearObservations(RadarObsList& list)
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
} // FusionDatabase::clearObservations

void
FusionDatabase::dumpRadars()
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

void
FusionDatabase::dumpXYZ()
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
