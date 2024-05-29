#include "rFusionDatabase.h"

using namespace rapio;

void
FusionDatabase::ingestNewData(Stage2Data& data, time_t cutoff, size_t& missingcounter, size_t& points, size_t& total)
{
  ProcessTimer timer("Ingest Source");

  // Metadata info
  std::string name      = data.getRadarName();
  std::string aTypeName = data.getTypeName();
  Time dataTime         = data.getTime();
  const size_t xBase    = data.getXBase();
  const size_t yBase    = data.getYBase();

  //    LogInfo("Incoming stage2 data for " << name << " " << aTypeName << "\n");

  // Get a current source list
  // Note: This should be a reference or you'll copy
  auto radarPtr = getSourceList(name);
  auto& radar   = *radarPtr;

  radar.myTime = dataTime;

  // Could store shorts and then do a move forward pass in output
  const time_t t = radar.myTime.getSecondsSinceEpoch();

  // Get a brand new source list
  auto newSourcePtr = getNewSourceList("newone");
  auto& newSource   = *newSourcePtr;

  // Read the source list, marking x,y,z found
  myHaves.clearAllBits();

  float v, w;
  short x, y, z;

  missingcounter = 0;
  points         = 0;
  total = 0;
  while (data.get(v, w, x, y, z)) {
    total++;
    x += xBase;
    y += yBase;

    if ((x >= myNumX) ||
      (y >= myNumY) ||
      (z >= myNumZ))
    {
      LogSevere(
        "Getting stage2 x,y,z values out of range of current grid: " << x << ", " << y << ", " << z << " and (" << myNumX << ", " << myNumY << ", " << myNumZ <<
          ")\n");
      break;
    }
    if (v == Constants::MissingData) {
      addMissing(newSource, x, y, z, t); // update mask
      missingcounter++;
    } else {
      addObservation(newSource, v, w, x, y, z, t);
      points++;
    }
  }
  LogInfo(timer << "\n");

  {
    ProcessTimer fail("Merge source");
    mergeObservations(radarPtr, newSourcePtr, cutoff);
    LogInfo(fail << "\n");
  }
} // FusionDatabase::ingestNewData

void
FusionDatabase::mergeTo(std::shared_ptr<LLHGridN2D> cache, const time_t cutoff, size_t offsetX, size_t offsetY)
{
  ProcessTimer test("Merging XYZ tree\n");

  // Use the coordinates of the cache in case it's a subgrid/tile
  // and not a full grid
  const size_t gridZ = cache->getNumLayers();
  const size_t gridY = cache->getNumLats(); // dim 0
  const size_t gridX = cache->getNumLons(); // dim 1

  // -------------------------------------------------------------
  // Accumulation pass...add up all numerators and denominators
  // of a weighted average sum.  Parts of which were generated by
  // multiple rFusion1 algorithms.

  // We'll still order by z, since we'll probably thread around it
  for (size_t z = 0; z < gridZ; z++) {
    std::shared_ptr<LatLonGrid> output = cache->get(z);

    // Accumulate 2D weights
    auto w = output->getFloat2D("weights");
    w->fill(0);
    auto& wa = output->getFloat2DRef("weights");

    // Accumulate 2D masks (when storing per radar).
    // auto m = output->getByte2D("masks");
    // m->fill(0);
    // auto& ma = output->getByte2DRef("masks");

    // Accumulate 2D values (borrow final output to save RAM)
    auto gridtestP = output->getFloat2D();
    gridtestP->fill(0);
    auto& gridtest = output->getFloat2DRef();

    // Here's the genius, we don't care about x,y,z ordering...we accumulate
    // the weights and values randomly...
    for (auto it = myObservationManager.begin(); it != myObservationManager.end(); ++it) {
      auto &r = *(it->second);

      // Missing observations just set the mask flag (background)
      // for (auto& m:r.myAMObs[z]) {
      //  ma[m.y][m.x] = 1;
      // }

      // Value observations accumulate values and weights
      for (auto& v:r.myAObs[z]) {
        // Since we can be a tile/partition, shifts global to partition coordinates
        // atX and atY are local coordinates in the partition
        // So we clip global to the area we cover
        const int atX = v.x - offsetX;
        const int atY = v.y - offsetY;
        if ((atX < 0) || (atY < 0) || (atX >= gridX) || (atY >= gridY)) {
          continue;
        }
        gridtest[atY][atX] += v.v;
        wa[atY][atX]       += v.w;
      }
    }
  }

  // -------------------------------------------------------------
  // Finialization pass, divide all values/weights and handle mask
  for (size_t z = 0; z < gridZ; z++) {
    std::shared_ptr<LatLonGrid> output = cache->get(z);
    auto& wa = output->getFloat2DRef("weights");
    // auto& ma       = output->getByte2DRef("masks");
    auto& gridtest = output->getFloat2DRef();
    for (size_t x = 0; x < gridX; x++) { // x currently LON for stage2 right..so xy swapped
      for (size_t y = 0; y < gridY; y++) {
        // if no values with enough weight...
        if (wa[y][x] < 0.0000001) {
          // Use the missing flag array....
          // if (ma[y][x] > 0) {
          // FIXME: Feel like with some ordering could skip the indexing calculation
          if (myMissings[myHaves.getIndex3D(x, y, z)] >= cutoff) {
            gridtest[y][x] = Constants::MissingData;
          } else {
            gridtest[y][x] = Constants::DataUnavailable;
          }
          continue;
        }

        // So we have weights, values....divide them to get total
        gridtest[y][x] /= wa[y][x];
      }
    }
  }

  LogInfo(test);
} // FusionDatabase::mergeTo

void
FusionDatabase::timePurge(Time atTime, TimeDuration d)
{
  // Time to expire data
  const Time cutoffTime = atTime - d;
  const time_t cutoff   = cutoffTime.getSecondsSinceEpoch();

  // For each source, purge times...
  for (auto it = myObservationManager.begin(); it != myObservationManager.end(); ++it) {
    it->second->timePurge(cutoff);
  }
} // FusionDatabase::timePurge

// SourceList&
std::shared_ptr<SourceList>
FusionDatabase::getSourceList(const std::string& name)
{
  return myObservationManager.getSourceList(name);
}

void
FusionDatabase::addObservation(SourceList& list, float v, float w, size_t x, size_t y, size_t z, time_t t)
{
  // -----------------------------------------
  // add to the source list
  list.addObservation(x, y, z, v, w, t);

  // Mark that we have this point
  myHaves.set13D(x, y, z);
}

void
FusionDatabase::addMissing(SourceList& list, size_t x, size_t y, size_t z, time_t t)
{
  // -----------------------------------------
  // add to the source missing list
  //  list.addMissing(x, y, z, t);
  // Add to global missing array mask
  size_t i = myHaves.getIndex3D(x, y, z);

  if (myMissings[i] < t) { // Always update to latest time from all
    myMissings[i] = t;
  }

  // Mark that we have this point
  // myHaves.set13D(x, y, z); // still 'expire' old valid values replaced by missing now (moving storm)
  myHaves.set1(i); // still 'expire' old valid values replaced by missing now (moving storm)
}

void
FusionDatabase::mergeObservations(std::shared_ptr<SourceList> oldSourcePtr,
  std::shared_ptr<SourceList> newSourcePtr, const time_t cutoff)
{
  auto& oldSource = *oldSourcePtr;
  auto& newSource = *newSourcePtr;

  // Make list have same identifiers..
  newSource.myName = oldSource.myName;
  newSource.myID   = oldSource.myID;
  newSource.myTime = oldSource.myTime; // deprecated

  // Adding up old sizes.  This could be a running total maybe
  // since even this takes time.
  size_t hadSize = 0;
  size_t newSize = 0;

  for (size_t l = 0; l < myNumZ; ++l) {
    hadSize += oldSource.myAObs[l].size(); // old points size
    hadSize += oldSource.myAMObs[l].size();
    newSize += newSource.myAObs[l].size(); // new incoming points
    newSize += newSource.myAMObs[l].size();
  }

  // Add up sizes in the new source
  size_t oldRestore = 0;
  size_t timePurged = 0;

  // Merge back old values not in new
  oldSource.unionMerge(newSource, myHaves, cutoff, timePurged, oldRestore);

  // And make it the new one
  myObservationManager.setSourceList(newSource.myID, newSourcePtr);

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

  // For all sources
  for (auto it = myObservationManager.begin(); it != myObservationManager.end(); ++it) {
    auto &r = *(it->second);

    // Get size of points.  Probably should be a method
    // I think we keep a running counter save these loops
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
    counter  += numObs;
    mcounter += numMObs;

    // Size is the actual capacity of vector
    sizeCounter += (numObsCap * (sizeof(VObservation)));
    sizeCounter += (numMObsCap * (sizeof(MObservation)));

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
  // Not sure how to guess this in new way yet
  //  LogInfo("X,Y,Z Coverage: " << myXYZs.getPercentFull() << "%\n");
} // FusionDatabase::dumpSources
