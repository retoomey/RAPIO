#include "rFusionDatabase.h"
#include "rProcessTimer.h"
#include "rArith.h"

using namespace rapio;

void
FusionDatabase::ingestNewData(std::shared_ptr<BinaryTable> data, time_t cutoff, size_t& missingcounter, size_t& points, size_t& total)
{
  ProcessTimer timer("Ingest Source");
  
  auto windTable   = std::dynamic_pointer_cast<WindBinaryTable>(data);
  auto fusionTable = std::dynamic_pointer_cast<FusionBinaryTable>(data);

  // 1. SAFELY IDENTIFY AND LOCK PAYLOAD FIRST
  if (windTable) {
    if (!myObservationManager.lockPayloadSize(5)) {
        fLogSevere("Payload mismatch! Database locked to size {}, but incoming data is size 5", 
                   myObservationManager.getPayloadSize());
        return;
    }
  } else if (fusionTable) {
    if (!myObservationManager.lockPayloadSize(2)) {
        fLogSevere("Payload mismatch! Database locked to size {}, but incoming data is size 2", 
                   myObservationManager.getPayloadSize());
        return;
    }
  } else {
    fLogSevere("Unrecognized BinaryTable type provided to FusionDatabase.");
    return;
  }

  // 2. NOW GRAB THE METADATA AND ALLOCATE THE MEMORY LISTS
  std::string name;
  std::string aTypeName;
  data->getString("Radarname", name);
  data->getString("Typename", aTypeName);

  Time dataTime               = data->getTime();
  const bool dataNoMissingSet = data->getUseMissingAsUnavailable();

  long xBase = 0, yBase = 0;
  data->getLong("xBase", xBase);
  data->getLong("yBase", yBase);

  // These will now safely generate the correctly typed and correctly sized PayloadSourceList
  auto radarPtr = getSourceList(name);
  auto& radar   = *radarPtr;
  radar.myTime  = dataTime;
  const time_t t = radar.myTime.getSecondsSinceEpoch();

  auto newSourcePtr = getNewSourceList("newone");
  auto& newSource   = *newSourcePtr;
  myHaves.clearAllBits();

  missingcounter = 0;
  points         = 0;
  total          = 0;

  // 3. INGESTION
  if (windTable) {
    auto payloadSource = std::static_pointer_cast<PayloadSourceList<5>>(newSourcePtr);
    short x, y;
    char z;
    const size_t valSize = windTable->getValueSize();

    for (size_t i = 0; i < valSize; ++i) {
      total++;
      x = windTable->myXs[i] + xBase;
      y = windTable->myYs[i] + yBase;
      z = windTable->myZs[i];

      if ((static_cast<size_t>(x) >= myNumX) ||
          (static_cast<size_t>(y) >= myNumY) ||
          (static_cast<size_t>(z) >= myNumZ))
      {
        fLogSevere("Getting stage2 x,y,z values out of range of current grid: {}, {}, {} and ({}, {}, {})", 
                   x, y, z, myNumX, myNumY, myNumZ);
        break;
      }

      std::array<float, 5> payload = {
        windTable->myM11[i],
        windTable->myM22[i],
        windTable->myM12[i],
        windTable->myP1[i],
        windTable->myP2[i]
      };

      payloadSource->addObservation(x, y, z, payload, t);
      myHaves.set13D(x, y, z);
      points++;
    }

    const size_t missSize = windTable->getMissingSize();
    for (size_t i = 0; i < missSize; ++i) {
      short startX = windTable->myXMissings[i] + xBase;
      short startY = windTable->myYMissings[i] + yBase;
      char startZ  = windTable->myZMissings[i];
      short length = windTable->myLMissings[i];

      // CRITICAL NEW BOUNDS CHECKS
      if (static_cast<size_t>(startY) >= myNumY || static_cast<size_t>(startZ) >= myNumZ) {
          continue;
      }

      for (short l = 0; l < length; ++l) {
        short curX = startX + l;
        if (static_cast<size_t>(curX) < myNumX) {
          addMissing(newSource, curX, startY, startZ, t, dataNoMissingSet);
          missingcounter++;
        }
      }
    }

  } else if (fusionTable) {
    auto payloadSource = std::static_pointer_cast<PayloadSourceList<2>>(newSourcePtr);
    float v, w;
    short x, y, z;
    
    while (fusionTable->get(v, w, x, y, z)) {
      total++;
      x += xBase;
      y += yBase;
      
      if ((static_cast<size_t>(x) >= myNumX) ||
          (static_cast<size_t>(y) >= myNumY) ||
          (static_cast<size_t>(z) >= myNumZ))
      {
        fLogSevere("Getting stage2 x,y,z values out of range of current grid: {}, {}, {} and ({}, {}, {})", 
                   x, y, z, myNumX, myNumY, myNumZ);
        break;
      }
      
      if (v == Constants::MissingData) {
        addMissing(newSource, x, y, z, t, dataNoMissingSet);
        missingcounter++;
      } else {
        std::array<float, 2> payload = {v, w};
        payloadSource->addObservation(x, y, z, payload, t);
        myHaves.set13D(x, y, z);
        points++;
      }
    }
  }

  fLogInfo("{}", timer);
  {
    ProcessTimer fail("Merge source");
    mergeObservations(radarPtr, newSourcePtr, cutoff);
    fLogInfo("{}", fail);
  }
}

void
FusionDatabase::timePurge(Time atTime, TimeDuration d)
{
  const Time cutoffTime = atTime - d;
  const time_t cutoff   = cutoffTime.getSecondsSinceEpoch();
  for (auto it = myObservationManager.begin(); it != myObservationManager.end(); ++it) {
    it->second->timePurge(cutoff);
  }
}

void
FusionDatabase::addMissing(SourceListBase& list, size_t x, size_t y, size_t z, time_t t, bool dataNoMissingSet)
{
  list.addMissing(x, y, z, t); 
  size_t i = myHaves.getIndex3D(x, y, z);
  if (!dataNoMissingSet) {
    if (myMissings[i] < t) {
      myMissings[i] = t;
    }
  }
  myHaves.set1(i);
}

void
FusionDatabase::mergeObservations(std::shared_ptr<SourceListBase> oldSourcePtr,
  std::shared_ptr<SourceListBase> newSourcePtr, const time_t cutoff)
{
  auto& oldSource = *oldSourcePtr;
  auto& newSource = *newSourcePtr;
  newSource.myName = oldSource.myName;
  newSource.myID   = oldSource.myID;
  newSource.myTime = oldSource.myTime;

  size_t hadSize = 0;
  size_t newSize = 0;

  if (myObservationManager.getPayloadSize() == 5) {
      auto oldList = std::static_pointer_cast<PayloadSourceList<5>>(oldSourcePtr);
      auto newList = std::static_pointer_cast<PayloadSourceList<5>>(newSourcePtr);
      for (size_t l = 0; l < myNumZ; ++l) {
          hadSize += oldList->myObs[l].size() + oldList->myAMObs[l].size();
          newSize += newList->myObs[l].size() + newList->myAMObs[l].size();
      }
  } else {
      auto oldList = std::static_pointer_cast<PayloadSourceList<2>>(oldSourcePtr);
      auto newList = std::static_pointer_cast<PayloadSourceList<2>>(newSourcePtr);
      for (size_t l = 0; l < myNumZ; ++l) {
          hadSize += oldList->myObs[l].size() + oldList->myAMObs[l].size();
          newSize += newList->myObs[l].size() + newList->myAMObs[l].size();
      }
  }

  size_t oldRestore = 0;
  size_t timePurged = 0;
  oldSource.unionMerge(newSource, myHaves, cutoff, timePurged, oldRestore);
  
  myObservationManager.setSourceList(newSource.myID, newSourcePtr);
  
  fLogInfo("{} Had: {} New: {} Kept: {} Expired: {} Final: {}",
    newSource.myName, hadSize, newSize, oldRestore, timePurged, newSize + oldRestore);
}

void
FusionDatabase::dumpSources()
{
  // Don't calculate, etc. if not debug logging
  if (!wouldLogDebug()) {
    return;
  }
  
  size_t counter     = 0;
  size_t mcounter    = 0;
  size_t sizeCounter = 0;
  size_t obsDelta    = 0;
  
  // For all sources
  for (auto it = myObservationManager.begin(); it != myObservationManager.end(); ++it) {
    auto &r = *(it->second);

    // Get size of points.  Probably should be a method
    // I think we keep a running counter save these loops
    size_t numObs     = 0;
    size_t numMObs    = 0;
    size_t numObsCap  = 0;
    size_t numMObsCap = 0;

    if (myObservationManager.getPayloadSize() == 5) {
        auto pl = std::static_pointer_cast<PayloadSourceList<5>>(it->second);
        for (size_t l = 0; l < myNumZ; ++l) {
            numObs     += pl->myObs[l].size();
            numObsCap  += pl->myObs[l].capacity();
            numMObs    += pl->myAMObs[l].size();
            numMObsCap += pl->myAMObs[l].capacity();
        }
        sizeCounter += (numObsCap * sizeof(GenericObservation<5>));
        sizeCounter += (numMObsCap * sizeof(MObservation));
    } else {
        auto pl = std::static_pointer_cast<PayloadSourceList<2>>(it->second);
        for (size_t l = 0; l < myNumZ; ++l) {
            numObs     += pl->myObs[l].size();
            numObsCap  += pl->myObs[l].capacity();
            numMObs    += pl->myAMObs[l].size();
            numMObsCap += pl->myAMObs[l].capacity();
        }
        sizeCounter += (numObsCap * sizeof(GenericObservation<2>));
        sizeCounter += (numMObsCap * sizeof(MObservation));
    }

    fLogInfo("{}: {}: {} v. {} m. Latest: {}", static_cast<uint32_t>(r.myID), r.myName, numObs, numMObs, r.myTime);
    counter  += numObs;
    mcounter += numMObs;
    obsDelta += (numObsCap - numObs);
  }
  
  double vm, rssm;
  OS::getProcessSizeKB(vm, rssm);
  vm   *= 1024;
  rssm *= 1024;
  
  fLogDebug("Total: {} v. {} m. ({}) ~RAM: {} {} , VWaste: {}",
    counter, mcounter, counter + mcounter,
    Strings::formatBytes(sizeCounter), Strings::formatBytes(rssm), Strings::formatBytes(obsDelta));
}
