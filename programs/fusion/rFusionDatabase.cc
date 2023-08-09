#include "rFusionDatabase.h"

using namespace rapio;

std::shared_ptr<std::unordered_set<size_t> > FusionDatabase::myMarked = nullptr;

// ---------------------------------------
// First merge
// This will become a plug-in probably for various merging options
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
  const Time cutoffTime = atTime - d;
  const time_t cutoff   = cutoffTime.getSecondsSinceEpoch();

  for (auto it = myObservationManager.begin(); it != myObservationManager.end(); ++it) {
    auto &r = it->second;
    // We can expire by merging with ourselves..but this think will take too long
    // in operations.  We don't need to do the merge unless it's possible we have a value
    // outside a minimum timestamp, which we could store in the list.  Remember
    // we drop old values when a new value comes in.

    // FIXME: dedicated method, optimize taking into account new data is time purging
    // already.

    // Hack for moment.  Any 'old' not cutoff get pushed into the newSource
    myMarked = std::make_shared<std::unordered_set<size_t> >();
    SourceList newSource = getNewSourceList("newone");
    mergeObservations(r, newSource, cutoff);
    myMarked = nullptr;
  }
} // FusionDatabase::timePurge

SourceList&
FusionDatabase::getSourceList(const std::string& name)
{
  return myObservationManager.getSourceList(name);
}

void
FusionDatabase::addObservation(SourceList& list, float v, float w, size_t x, size_t y, size_t z, time_t t)
{
  // -----------------------------------------
  // add to the source list
  //
  list.myObs.push_back(Observation(x, y, z, v, w, t));

  // We'll merge source so we need to know to remove old values now replaced by new observations
  // Use the missing grid here to get i, should be fine
  size_t i = myMissings.getIndex3D(x, y, z); // FIXME: passing i might be faster

  myMarked->insert(i);
}

void
FusionDatabase::addMissing(SourceList& source, size_t x, size_t y, size_t z, time_t time)
{
  size_t i = myMissings.getIndex3D(x, y, z); // FIXME: passing i might be faster

  myMissings.set(i, time); // FIXME: Humm technically we should update iff the new time if greater
  myMarked->insert(i);
}

void
FusionDatabase::linkObservations(SourceList& list)
{
  size_t s = 0;

  for (auto& o: list.myObs) {
    size_t i = myXYZs.getIndex3D(o.x, o.y, o.z);
    XYZObsList * node = myXYZs.get(i); // Sparse so could be null

    auto * at = &list.myObs[s]; // only good if vector doesn't move

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
    s++;
  }
}

void
FusionDatabase::unlinkObservations(SourceList& list)
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
} // FusionDatabase::unlinkObservations

void
FusionDatabase::clearObservations(SourceList& list)
{
  // Update xyz tree removing each observation back reference
  unlinkObservations(list);
  list.myObs.clear();
} // FusionDatabase::clearObservations

void
FusionDatabase::mergeObservations(SourceList& oldSource, SourceList& newSource, const time_t cutoff)
{
  // Make list have same identifiers..
  newSource.myName = oldSource.myName;
  newSource.myID   = oldSource.myID;
  newSource.myTime = oldSource.myTime; // deprecated

  // Remove all old source back references in the x, y, z tree
  // but keep the observations
  size_t hadSize = oldSource.myObs.size();

  unlinkObservations(oldSource);

  // Mark/keep track of x,y,z values we have in the new source.
  // We'll put back old values that don't have new values, which
  // basically replaces all x,y,z values that are new
  // addMissing already marked some
  auto& have = *myMarked;

  for (const auto& n: newSource.myObs) {
    size_t i = myXYZs.getIndex3D(n.x, n.y, n.z);
    have.insert(i);
  }

  // Put back in the old source not in the new list that are still in the cutoff time
  // We want the list finalized before using x, y, z back pointer references,
  // because changing vectors can move them in memory
  size_t oldRestore = 0;
  size_t timePurged = 0;
  size_t newSize    = newSource.myObs.size();

  for (auto& o: oldSource.myObs) {
    // If not a value already in new source, add the old source value
    size_t i = myXYZs.getIndex3D(o.x, o.y, o.z);
    if (have.find(i) == have.end()) {
      if (o.t >= cutoff) { // Go ahead expire.  We already removed from x, y, z so it's safe
        newSource.myObs.push_back(o);
        oldRestore++;
      } else {
        timePurged++;
      }
    }
  }

  // Clean it up, don't need to store it
  have.clear();

  // Assign first, allow memory moving if needed
  myObservationManager.setSourceListRef(newSource.myID, newSource);

  // -----------------------------------------
  // Update xyz tree the new list. This was time purged during creation
  auto& theSource = myObservationManager.getSourceListRef(newSource.myID);

  linkObservations(theSource);

  LogInfo(newSource.myName << " Had: " << hadSize << " New: " <<
    newSize << " Kept: " << oldRestore << " Expired: " <<
    timePurged << " Final: " << theSource.myObs.size() << "\n");
} // FusionDatabase::mergeObservations

void
FusionDatabase::dumpSources()
{
  size_t counter     = 0;
  size_t sizeCounter = 0;

  size_t obsDelta = 0;

  for (auto it = myObservationManager.begin(); it != myObservationManager.end(); ++it) {
    auto &r = it->second;
    LogInfo(
      r.myID << ": " << r.myName << ": " << r.myObs.size() <<
        " obs. Latest: " << r.myTime.getString("%H:%M:%S") << "\n");
    counter += r.myObs.size();
    // Size is the actual capacity of vector
    sizeCounter += (r.myObs.capacity() * (sizeof(Observation)));
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
