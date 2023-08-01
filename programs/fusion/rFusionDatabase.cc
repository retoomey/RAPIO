#include "rFusionDatabase.h"

using namespace rapio;

// ---------------------------------------
// First merge
//
void
FusionDatabase::mergeTo(std::shared_ptr<LLHGridN2D> cache)
{
  static bool firstTime = true;
  size_t failed         = 0;
  size_t failed2        = 0;
  size_t missingHit     = 0;

  ProcessTimer test("Merging XYZ tree\n");

  for (size_t z = 0; z < myNumZ; z++) {
    std::shared_ptr<LatLonGrid> output = cache->get(z);

    if (output == nullptr) {
      LogSevere("Failed to get layer " << z << " from LLH cache\n");
      return;
    }

    auto& gridtest = output->getFloat2DRef();
    for (size_t x = 0; x < myNumX; x++) { // x currently LON for stage2 right..so xy swapped
      for (size_t y = 0; y < myNumY; y++) {
        // ----------------------------------------------
        // Mask calculation (special snowflake due to sizes)
        float v    = Constants::DataUnavailable; // mask
        size_t i2  = myMissings.getIndex3D(x, y, z);
        auto mnode = myMissings.get(i2);
        if (mnode != nullptr) {
          v = Constants::MissingData;
          missingHit++;
        }

        // ----------------------------------------------
        // Silly simple weighted average of everything we got
        // This is the logic for combining values
        size_t i          = myXYZs.getIndex3D(x, y, z);
        auto node         = myXYZs.get(i); // Sparse so could be null
        float weightedSum = 0.0;
        float totalWeight = 0.0;
        if (node != nullptr) {
          bool haveValues = false;
          for (const auto& n: node->obs) {
            // Quick check for now
            if (n.myAt == nullptr) {
              failed += 1;
              continue;
            }
            const auto &data = *(n.myAt); // Pointer should be good

            weightedSum += data.v * data.w;
            totalWeight += data.w;
            haveValues   = true;
          }

          // Final v
          if (haveValues && (totalWeight > 0.001)) {
            v = weightedSum / totalWeight;
          }
        }

        gridtest[y][x] = v;
        // ----------------------------------------------
      }
    }
  }
  if ((failed > 0) || (failed2 > 0)) {
    LogSevere("Failed hits: " << failed << " and " << failed2 << "\n");
  }
  LogSevere(">>MISSING COUNT: " << missingHit << "\n");
  firstTime = false;
} // FusionDatabase::mergeTo

void
FusionDatabase::timePurge(Time atTime, TimeDuration d)
{
  // Time to expire data
  const Time cutoff = atTime - d;

  // Extra time to expire from our list (so we can see expires)
  Time cutoffList = atTime - (d + TimeDuration::Minutes(5));

  LogInfo("Cutoff time: " << cutoff.getString("%H:%M:%S") << " from " << atTime.getString("%H:%M:%S") << "\n");

  // Since we have one time currently per tilt/radar, this is just
  // clearing the observations for any tilt/radar outside the time window
  for (auto it = myRadarObsManager.begin(); it != myRadarObsManager.end(); ++it) {
    auto &r = it->second;

    if (r.myTime < cutoff) {
      // Expire observations for this radar list
      if (r.myObs.size() > 0) {
        LogInfo("--->" << r.myName << " " << r.myElevDegs << " has expired. " << r.myTime.getString("%H:%M:%S") <<
          "\n");
        clearObservations(r);
        r.myExpired = true;
      }

      // Remove radar list from the list (so we don't infinitely grow)
      if (r.myTime < cutoffList) { // also remove from list
        myRadarObsManager.remove(it->second);
        LogInfo("--->" << r.myName << " " << r.myElevDegs << " has been removed.\n");
      }
    }
  }
}

RadarObsList&
FusionDatabase::getRadar(const std::string& radarname, float elevDegs)
{
  return myRadarObsManager.getRadar(radarname, elevDegs);
}

void
FusionDatabase::addObservation(RadarObsList& list, float v, float w, size_t x, size_t y, size_t z)
{
  // -----------------------------------------
  // Update radar tree
  //
  size_t s = list.myObs.size();

  list.myObs.push_back(RadarObs(x, y, z, v, w));
  auto * at = &list.myObs[s]; // only good if vector doesn't move

  // -----------------------------------------
  // Update xyz tree
  //
  // This is slow for entire x,y,z.  Since radar will be much smaller
  // in theory it will be fast enough
  size_t i = myXYZs.getIndex3D(x, y, z);
  XYZObsList * node = myXYZs.get(i); // Sparse so could be null

  if (node == nullptr) {
    XYZObsList newlist;
    node = myXYZs.set(i, newlist); // bleh means sparse vector always grows, never shrinks.  Need to handle that
    // though to be fair I don't think w2merger shrinks either
    if (node != nullptr) {
      node->obs.push_back(XYZObs(list.myID, s, at));
    }
  } else {
    node->obs.push_back(XYZObs(list.myID, s, at));
  }
} // FusionDatabase::addObservation

void
FusionDatabase::addMissing(size_t x, size_t y, size_t z, time_t time)
{
  size_t i = myMissings.getIndex3D(x, y, z); // FIXME: passing i might be faster

  myMissings.set(i, time); // FIXME: Humm technically we should update iff the new time if greater
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
        const auto& o = list.myObs;
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
FusionDatabase::reserveSizes(RadarObsList& list, size_t values, size_t missings)
{
  if (list.myObs.size() > 0) {
    LogSevere("Trying to reserve sizes for a radar containing " << list.myObs.size() << " observations!\n");
    return;
  }
  list.myObs.reserve(values); // no size change but reserve the memory. This 'should' keep vector from
                              // moving and breaking xyz back reference pointers
}

void
FusionDatabase::dumpRadars()
{
  size_t counter     = 0;
  size_t sizeCounter = 0;

  size_t obsDelta = 0;

  for (auto it = myRadarObsManager.begin(); it != myRadarObsManager.end(); ++it) {
    auto &r = it->second;
    std::string extra = r.myExpired ? " EXPIRED " : "";
    LogInfo(
      r.myID << ": " << r.myName << " " << r.myElevDegs << ": " << r.myObs.size() <<
        " observations at " << r.myTime.getString("%H:%M:%S") << extra << "\n");
    counter += r.myObs.size();
    // Size is the actual capacity of vector
    sizeCounter += (r.myObs.capacity() * (sizeof(RadarObs)));
    // Guess with size times XYZObs.  We'd have to actually check each vector of XYZObs to get 100% here
    sizeCounter += (r.myObs.size() * sizeof(XYZObs));
    // Different in stored vs allocated 'should' be minor but checking
    obsDelta += (r.myObs.capacity() - r.myObs.size());
  }
  LogInfo("Total observations: " << counter << " Guessed RAM: " << Strings::formatBytes(sizeCounter)
                                 << " , VWaste: " << Strings::formatBytes(obsDelta) << "\n");
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
