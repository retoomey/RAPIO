#include "rFusionDatabase.h"

using namespace rapio;

std::shared_ptr<std::unordered_set<size_t> > FusionDatabase::myMarked = nullptr;

std::shared_ptr<SparseVector<size_t> > FusionDatabase::myMarked2 = nullptr;

// ---------------------------------------
// First merge
// This will become a plug-in probably for various merging options
//
void
FusionDatabase::mergeTo(std::shared_ptr<LLHGridN2D> cache, const time_t cutoff)
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
        /*
         *      // ----------------------------------------------
         *      // Mask calculation (special snowflake due to sizes)
         *      float v    = Constants::DataUnavailable; // mask
         *      size_t i2  = myMissings.getIndex3D(x, y, z);
         *      auto mnode = myMissings.get(i2);
         *      if (mnode != nullptr) {
         *        if (*mnode >= cutoff) {
         *          v = Constants::MissingData;
         *          missingHit++;
         *        }
         *      }
         */
        // ----------------------------------------------
        // Mask calculation (special snowflake due to sizes)
        float v = Constants::DataUnavailable; // mask

        /*
         *      size_t i2  = myMissings.getIndex3D(x, y, z);
         *      auto mnode = myMissings.get(i2);
         *      if (mnode != nullptr) {
         *        if (*mnode >= cutoff) {
         *          v = Constants::MissingData;
         *          missingHit++;
         *        }
         *      }
         */

        // ----------------------------------------------
        // Silly simple weighted average of everything we got
        // This is the logic for combining values
        size_t i          = myXYZs.getIndex3D(x, y, z);
        auto node         = myXYZs.get(i); // Sparse so could be null
        float weightedSum = 0.0;
        float totalWeight = 0.0;
        if (node != nullptr) {
          bool haveValues = false;
          // for (const auto& n: node->obs) {
          for (const auto& n: *node) {
            // Quick check for now
            if (n.myAt == nullptr) {
              failed += 1;
              continue;
            }

            if (n.isMissing == true) { // Skip missing values
              v = Constants::MissingData;
              missingHit++;
              continue;
            }

            const auto &data = *((VObservation *) (n.myAt)); // Pointer should be good

            //  const auto& wi = 1.0/std::pow(data.w, 2);
            // weightedSum += data.v * wi;
            weightedSum += data.v;
            totalWeight += data.w;
            haveValues   = true;
            #if 0
            // data.w is range...so 0 range is actually weight 1
            // and we pin the weight to max
            if (data.w < 0.001) {
              weightedSum += data.v; // 1.0/1
              totalWeight += 1.0;
              haveValues   = true;
            } else {
              const auto& wi = 1.0 / std::pow(data.w, 2);
              weightedSum += data.v * wi;
              totalWeight += wi;
              haveValues   = true;
            }
            #endif // if 0
          }

          // Final v
          if (haveValues) {
            v = weightedSum / totalWeight;
            // hack precision for moment to .5
            int intValue        = static_cast<int>(v * 2);
            int roundedIntValue = std::round(intValue);
            v = static_cast<float>(roundedIntValue) / 2.0f;
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
    //  myMarked2 = std::make_shared<SparseVector<size_t> >(SparseVector<size_t>({myNumX, myNumY, myNumZ}));
    auto newSource = getNewSourceList("newone");
    mergeObservations(r, newSource, cutoff);
    myMarked = nullptr;
    //   myMarked2 = nullptr;
  }
} // FusionDatabase::timePurge

// SourceList&
std::shared_ptr<SourceList>
FusionDatabase::getSourceList(const std::string& name)
{
  return myObservationManager.getSourceList(name);
}

void
FusionDatabase::addObservation(SourceList& list, float v, float w, float w2, size_t x, size_t y, size_t z, time_t t)
{
  // -----------------------------------------
  // add to the source list
  //
  //  list.myObs.push_back(VObservation(x, y, z, v, w, t, w2));
  list.addObservation(x, y, z, v, w, t, w2);

  // We'll merge source so we need to know to remove old values now replaced by new observations
  // Use the missing grid here to get i, should be fine
  // size_t i = myMissings.getIndex3D(x, y, z); // FIXME: passing i might be faster
  size_t i = myXYZs.getIndex3D(x, y, z); // FIXME: passing i might be faster

  myMarked->insert(i);
}

void
FusionDatabase::addMissing(SourceList& list, float w, float w2, size_t x, size_t y, size_t z, time_t t)
{
  // -----------------------------------------
  // add to the source missing list
  //
  //  list.myMObs.push_back(MObservation(x, y, z, t, w2));
  list.addMissing(x, y, z, t, w2);

  // Old time mark way..keep for moment
  // size_t i = myMissings.getIndex3D(x, y, z); // FIXME: passing i might be faster
  size_t i = myXYZs.getIndex3D(x, y, z); // FIXME: passing i might be faster

  //  myMissings.set(i, t); // FIXME: Humm technically we should update iff the new time if greater
  myMarked->insert(i);
}

/*
 * void
 * FusionDatabase::linkObservations(SourceList& list)
 * {
 * //  linkVector(list.myObs, list.myID, false);
 * //  linkVector(list.myMObs, list.myID, true);
 *
 * // Humm a visitor pattern we could let list do it maybe?
 * for (size_t l = 0; l < myNumZ; ++l) {
 *  linkVector(list.myAObs[l], list.myID, false, l);
 *  linkVector(list.myAMObs[l], list.myID, true, l);
 * }
 * } // FusionDatabase::linkObservations
 */

/*
 * void
 * FusionDatabase::unlinkObservations(SourceList& list)
 * {
 * //  unlinkVector(list.myObs); Using o.z
 * //  unlinkVector(list.myMObs);
 *
 * for (size_t l = 0; l < myNumZ; ++l) {
 *  unlinkVector(list.myAObs[l], l);
 *  unlinkVector(list.myAMObs[l], l);
 * }
 * } // FusionDatabase::unlinkObservations
 */

/*
 * void
 * FusionDatabase::clearObservations(SourceList& list)
 * {
 * // Update xyz tree removing each observation back reference
 * unlinkObservations(list);
 * list.clear();
 * } // FusionDatabase::clearObservations
 */

void
// FusionDatabase::mergeObservations(SourceList& oldSource, SourceList& newSource, const time_t cutoff)
FusionDatabase::mergeObservations(std::shared_ptr<SourceList> oldSourcePtr,
  std::shared_ptr<SourceList> newSourcePtr, const time_t cutoff)
{
  auto& oldSource = *oldSourcePtr;
  auto& newSource = *newSourcePtr;

  // Make list have same identifiers..
  newSource.myName = oldSource.myName;
  newSource.myID   = oldSource.myID;
  newSource.myTime = oldSource.myTime; // deprecated

  // Remove all old source back references in the x, y, z tree
  // but keep the observations
  //  size_t hadSize  = oldSource.myObs.size();
  //  size_t hadSize2 = oldSource.myMObs.size();

  ////   unlinkObservations(oldSource);

  // ^^^ same code but adding up...combine?
  size_t hadSize  = 0;
  size_t hadSize2 = 0;

  for (size_t l = 0; l < myNumZ; ++l) {
    hadSize += oldSource.myAObs[l].size();
    //    std::cout << "Debug: " << l << " yields " << newSource.myAObs[l].size() << ", " << newSource.myAMObs[l].size() << "\n";
    hadSize += oldSource.myAMObs[l].size();
    //    unlinkVector(oldSource.myAObs[l], l);
    //    unlinkVector(oldSource.myAMObs[l], l);
  }

  // Mark/keep track of x,y,z values we have in the new source.
  // We'll put back old values that don't have new values, which
  // basically replaces all x,y,z values that are new
  // addMissing already marked some
  auto& have  = *myMarked;
  auto& have2 = *myMarked2;

  #if 0
  for (const auto& n: newSource.myObs) {
    size_t i = myXYZs.getIndex3D(n.x, n.y, n.z);
    have.insert(i);
  }

  for (const auto& n: newSource.myMObs) {
    size_t i = myXYZs.getIndex3D(n.x, n.y, n.z);
    have.insert(i);
  }
  #endif

  // Marked for each layer
  // Why not link here?  The reason is that adding the old
  // values to us can change pointer addresses (move/reallocation)
  // So our observation memory locations are not yet final

  for (size_t l = 0; l < myNumZ; ++l) {
    for (auto& n: newSource.myAObs[l]) {
      size_t i = myXYZs.getIndex3D(n.x, n.y, l);
      have.insert(i);
    }
    for (auto& n: newSource.myAMObs[l]) {
      size_t i = myXYZs.getIndex3D(n.x, n.y, l);
      have.insert(i);
    }
  }

  // Put back in the old source not in the new list that are still in the cutoff time
  // We want the list finalized before using x, y, z back pointer references,
  // because changing vectors can move them in memory

  size_t oldRestore = 0;
  size_t timePurged = 0;
  #if 0
  size_t newSize = newSource.myObs.size();

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

  for (auto& o: oldSource.myMObs) {
    // If not a value already in new source, add the old source value
    size_t i = myXYZs.getIndex3D(o.x, o.y, o.z);
    if (have.find(i) == have.end()) {
      if (o.t >= cutoff) { // Go ahead expire.  We already removed from x, y, z so it's safe
        newSource.myMObs.push_back(o);
        oldRestore++;
      } else {
        timePurged++;
      }
    }
  }
  #endif // if 0

  size_t newSize = 0;
  for (size_t l = 0; l < myNumZ; ++l) {
    newSize += newSource.myAObs[l].size();  // new incoming points
    newSize += newSource.myAMObs[l].size(); // new incoming points
    // std::cout << "Debug: " << l << " yields " << newSource.myAObs[l].size() << ", " << newSource.myAMObs[l].size() << "\n";
  }

  // All this mess of loops to save RAM.
  // FIXME: Could macro it
  for (size_t l = 0; l < myNumZ; ++l) {
    for (auto& o: oldSource.myAObs[l]) {
      // If not a value already in new source, add the old source value
      size_t i = myXYZs.getIndex3D(o.x, o.y, l);
      if (have.find(i) == have.end()) {
        if (o.t >= cutoff) { // Go ahead expire.  We already removed from x, y, z so it's safe
          newSource.myAObs[l].push_back(o);
          have.insert(i); // have is all stuff that's in new
          oldRestore++;
        } else {
          timePurged++;
        }
      }
    }
    for (auto& o: oldSource.myAMObs[l]) {
      // If not a value already in new source, add the old source value
      size_t i = myXYZs.getIndex3D(o.x, o.y, l);
      if (have.find(i) == have.end()) {
        if (o.t >= cutoff) { // Go ahead expire.  We already removed from x, y, z so it's safe
          newSource.myAMObs[l].push_back(o);
          have.insert(i);
          oldRestore++;
        } else {
          timePurged++;
        }
      }
    }
  }

  // Clean it up, don't need to store it
  // have.clear();

  // Assign first, allow memory moving if needed
  // myObservationManager.setSourceListRef(newSource.myID, newSource);
  myObservationManager.setSourceList(newSource.myID, newSourcePtr);


  // Ok now..we:
  // 1.  Delete nodes in old source that we don't have in the new
  // 2.  Update all nodes in the new
  // for (size_t l = 0; l < myNumZ; ++l) {
  //  unlinkVector(list.myAObs[l], l);
  //  unlinkVector(list.myAMObs[l], l);
  // }
  // Delete nodes in the old source that we don't have in the new
  // this marks overlap nodes with nullptr for link to fill in
  for (size_t l = 0; l < myNumZ; ++l) {
    unlinkVector(oldSource.myAObs[l], l);
    unlinkVector(oldSource.myAMObs[l], l);
  }

  // 2.  Update all nodes in the new, including overlapping old ones
  for (size_t l = 0; l < myNumZ; ++l) {
    linkVector(newSource.myAObs[l], newSource.myID, false, l);
    linkVector(newSource.myAMObs[l], newSource.myID, true, l);
  }

  // -----------------------------------------
  // Update xyz tree the new list. This was time purged during creation
  // auto& theSource = myObservationManager.getSourceListRef(newSource.myID);

  // So all 'have' nodes need to be checked and pointers updated.
  // all 'not have' nodes in old source need to be deleted.
  // FIXME: does the source list ref there destory our SourceList?

  // Clean it up, don't need to store it
  have.clear();
  // GOOP  linkObservations(theSource);

  LogInfo(newSource.myName << " Had: " << hadSize << " New: " <<
    newSize << " Kept: " << oldRestore << " Expired: " <<
    timePurged << " Final: " << newSize + oldRestore << "\n");
} // FusionDatabase::mergeObservations

void
FusionDatabase::dumpSources()
{
  size_t counter     = 0;
  size_t mcounter    = 0;
  size_t sizeCounter = 0;

  size_t obsDelta = 0;

  // Size of the sparse vector of x,y,z pointers
  sizeCounter += myXYZs.deepsize();
  LogInfo("XYZ basesize: " << sizeCounter << " RAM: " << Strings::formatBytes(sizeCounter) << "\n");
  for (auto it = myObservationManager.begin(); it != myObservationManager.end(); ++it) {
    auto &r = *(it->second);

    // Get size of points.  Probably should be a method
    size_t numObs     = 0;
    size_t numMObs    = 0;
    size_t numObsCap  = 0;
    size_t numMObsCap = 0;
    for (size_t l = 0; l < myNumZ; ++l) {
      numObs     += r.myAObs[l].size();
      numObsCap  += r.myAObs[l].capacity();
      numMObs    += r.myAMObs[l].size();
      numMObsCap += r.myAMObs[l].capacity();
    }

    LogInfo(
      r.myID << ": " << r.myName << ": " << numObs <<
        " v. " << numMObs << " m. Latest: " << r.myTime.getString("%H:%M:%S") << "\n");
    // LogInfo("XYZObs size: " << sizeof(XYZObs) <<  " VObs: " << sizeof(VObservation) << " Mobs: " <<
    //    sizeof(MObservation) << "\n");
    counter  += numObs;
    mcounter += numMObs;
    // Size is the actual capacity of vector
    sizeCounter += (numObsCap * (sizeof(VObservation)));
    sizeCounter += (numMObsCap * (sizeof(MObservation)));
    // Guess with size times XYZObs.  We'd have to actually check each vector of XYZObs to get 100% here
    sizeCounter += (numObs * sizeof(XYZObs));
    sizeCounter += (numObs * sizeof(void *)); // back reference pointers
    // Different in stored vs allocated 'should' be minor but checking
    obsDelta += (numObsCap - numObs);
  }

  double vm, rssm;

  OS::getProcessSizeKB(vm, rssm);
  vm   *= 1024;
  rssm *= 1024; // need bytes for memory print

  LogInfo("Total: " << counter << " v. " << mcounter << " m. (" << counter + mcounter <<
    ") ~RAM: " << Strings::formatBytes(sizeCounter) << " " << Strings::formatBytes(rssm)
                    << " , VWaste: " << Strings::formatBytes(obsDelta) << "\n");
  LogInfo("X,Y,Z Coverage: " << myXYZs.getPercentFull() << "%\n");
} // FusionDatabase::dumpSources

void
FusionDatabase::dumpXYZ()
{
  ProcessTimer test("Scanning XYZ tree (long)\n");
  size_t counter   = 0;
  size_t nodecount = 0;
  size_t over4     = 0;

  for (size_t x = 0; x < myNumX; x++) {
    for (size_t y = 0; y < myNumY; y++) {
      for (size_t z = 0; z < myNumZ; z++) {
        size_t i  = myXYZs.getIndex3D(x, y, z);
        auto node = myXYZs.get(i); // Sparse so could be null
        if (node != nullptr) {
          nodecount += 1;
          counter   += node->size();
          // counter += node->obs.size();
          // if (node->obs.size() > 4) {
          if (node->size() > 4) {
            over4++;
          }
        }
      }
    }
  }
  LogInfo("Total XYZ scanned count is " << counter << "\n");
  LogInfo("Over 4 stored count is " << over4 << "\n");
  size_t full = (counter * sizeof(XYZObs)) + (nodecount * 16);

  LogInfo("Guess of current XYZ size: " << Strings::formatBytes(full) << "\n");
  LogInfo(test << "\n");
}
