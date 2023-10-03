#include "rFusion2.h"

#include "rStage2Data.h"
#include <vector>

using namespace rapio;

/** Merging requires some basic abilities.  This less data we
 * store per node the better due to the massive size
 * 1.  Need to find nodes with times matching for all XYZ
 *     This is for expiring old data at will.
 *     Current: O(N^3) search xyzStorage, delete nodes
 * 2.  Need to find nodes with radar/elevation matching
 *     Do we need elevation angle to actually merge?  It's a lot
 *     of extra data.
 * 3.  Need to iterate fast in x,y,z to get all nodes matching
 *     This is solved by xyz array with N nodes per location.
 */

void
RAPIOFusionTwoAlg::declareOptions(RAPIOOptions& o)
{
  o.setDescription(
    "RAPIO Fusion Stage Two Algorithm.  Gathers data from multiple radars for mergin values");
  o.setAuthors("Robert Toomey");

  // legacy use the t, b and s to define a grid
  o.declareLegacyGrid();

  // Default sync heartbeat to 2 mins
  // Format is seconds then mins
  o.setRequiredValue("sync", "0 */2 * * * *");
}

/** RAPIOAlgorithms process options on start up */
void
RAPIOFusionTwoAlg::processOptions(RAPIOOptions& o)
{
  o.getLegacyGrid(myFullGrid);
} // RAPIOFusionTwoAlg::processOptions

void
RAPIOFusionTwoAlg::createLLGCache(
  const std::string    & outputName,
  const std::string    & outputUnits,
  const LLCoverageArea & g)
{
  // NOTE: Tried it as a 3D array.  Due to memory fetching, etc. having N 2D arrays
  // turns out to be faster than 1 3D array.
  if (myLLGCache == nullptr) {
    ProcessTimer("Creating initial LLG value cache.");

    myLLGCache = LLHGridN2D::Create(outputName, outputUnits, Time(), g);
    myLLGCache->fillPrimary(Constants::DataUnavailable);
  }
}

void
RAPIOFusionTwoAlg::firstDataSetup(std::shared_ptr<Stage2Data> data)
{
  static bool setup = false;

  if (setup) { return; }

  setup      = true;
  myTypeName = data->getTypeName();

  // Generate output name and units.
  // FIXME: More control flags, maybe even name options
  myWriteCAPPIName   = "Fused2" + myTypeName;
  myWriteOutputUnits = data->getUnits();
  if (myWriteOutputUnits.empty()) {
    LogSevere("Units still wonky because of W2 bug, forcing dBZ at moment..\n");
    myWriteOutputUnits = "dBZ";
  }

  // Create working LLG cache CAPPI storage per height level
  // Note this will be full LLG in ram.
  // FIXME: We could try implementing a LLHGridN2D sparsely but then
  // we'd also have to do writer/reader work
  // if (myWriteLLG){ // 3 GB for conus oh yay
  createLLGCache(myWriteCAPPIName, myWriteOutputUnits, myFullGrid);
  // }

  // Finally, create the point cloud database with N observations per point
  myDatabase = std::make_shared<FusionDatabase>(myFullGrid.getNumX(), myFullGrid.getNumY(), myFullGrid.getNumZ());
  LogInfo(
    "Created XYZ array of " << myFullGrid.getNumX() << "*" << myFullGrid.getNumY() << "*" << myFullGrid.getNumZ() <<
      " size\n");
  // LogInfo("DATABASE IS " << (void*)(myDatabase.get()) << "\n");
}

void
RAPIOFusionTwoAlg::processNewData(rapio::RAPIOData& d)
{
  auto datasp = Stage2Data::receive(d); // Hide internal format in the stage2 data

  size_t missingcounter = 0;
  size_t points         = 0;
  size_t total = 0;

  if (datasp) {
    auto& data            = *datasp;
    std::string name      = data.getRadarName();
    std::string aTypeName = data.getTypeName();
    float elevDegs        = data.getElevationDegs(); // Got to go sorry
    Time dataTime         = data.getTime();

    LogInfo("Incoming stage2 data for " << name << " " << aTypeName << " " << elevDegs << "\n");

    // Initialize everything related to this radar
    firstDataSetup(datasp);

    // Check if incoming moment matches our single setup, otherwise we'd need
    // all the setup for each moment.  Which we 'could' do later maybe
    // Ok so we have things like ReflectivityDPQC and CASSF-CorrectedRefDPQC so it's up
    // to the user to not try to merge velocity and reflectivity, lol.
    // FIXME:  I think we'll add a units sanity check here on the data.
    // My concern is at some point someone will misconfigure and try to merge
    // Reflectivity with Velocity or something and we get crazy output
    // if (myTypeName != aTypeName) {
    //  LogSevere(
    //    "We are linked to moment '" << myTypeName << "', ignoring '" << name << "-" << aTypeName <<
    //      "'\n");
    //  return;
    // }

    // Keep a count of 'hits' in the output layer for now
    auto heightsKM = myFullGrid.getHeightsKM();
    std::vector<size_t> hitCount;
    hitCount.resize(heightsKM.size());

    const size_t xBase = data.getXBase();
    const size_t yBase = data.getYBase();

    auto& db = *myDatabase;

    // FIXME: We probably will have a db.ingestNewData() or something
    // Because this is mostly db interacting with itself.  If we want to implement
    // a cloud database this will have to be even more generic than this.

    // Note: This should be a reference or you'll copy
    auto radarPtr = db.getSourceList(name);
    auto& radar   = *radarPtr;

    // The time for all the current new observations
    radar.myTime = data.getTime(); // deprecated

    // Quick toss/expire incoming data, anything > cutoff time is too old
    const Time cutoffTime = myLastDataTime - myMaximumHistory; // FIXME: add util to time
    const time_t cutoff   = cutoffTime.getSecondsSinceEpoch();
    if (radar.myTime < cutoffTime) {
      LogSevere("Ignoring " << radar.myName << " OLD: " << radar.myTime.getString("%H:%M:%S") << "\n");
      return;
    }

    ProcessTimer timer("Ingest Source");

    // Could store shorts and then do a move forward pass in output
    const time_t t = radar.myTime.getSecondsSinceEpoch();

    // Stream read stage2 into a new observation list
    float v, w, w2;
    short x, y, z;

    // Add to a new list of observations or update missing mask
    // SourceList newSource = db.getNewSourceList("newone");
    auto newSourcePtr = db.getNewSourceList("newone");
    auto& newSource   = *newSourcePtr;

    // We use a shared ptr since set seems to be flaky about releasing memory
    // where clear() doesn't release the hash, and shrink_to_fit is not available
    // This is just flag for 'hit' values, might be better as a bitset.  We'll
    // revisit this later
    // FIXME: Should be hidden in the database
    db.myMarked = std::make_shared<std::unordered_set<size_t> >();

    auto& mark = *db.myMarked;

    while (data.get(v, w, w2, x, y, z)) {
      total++;
      hitCount[z]++;

      x += xBase; // FIXME: data.get should do this right? Not 100% sure yet
      y += yBase;

      if ((x >= myFullGrid.getNumX()) ||
        (y >= myFullGrid.getNumY()) ||
        (z >= myFullGrid.getNumZ()))
      {
        LogSevere("Getting stage2 x,y,z values out of range of current grid: " << x << ", " << y << ", " << z << "\n");
        break;
      }

      if (v == Constants::MissingData) {
        missingcounter++;
        db.addMissing(newSource, w, w2, x, y, z, t); // update mask
        continue;                                    // Skip storing missing values
      } else {
        points++;
      }

      // Point cloud
      db.addObservation(newSource, v, w, w2, x, y, z, t);
    }
    LogInfo(timer << "\n");

    // Now merge the newSource with the oldOne, expiring data points if needed
    // These can be overlapping sets.  Since x,y,z are created in order this 'could'
    // be optimized by double marching vs delete/add, but it would be confusing so eh.
    // FIXME: Can we do this fast enough is the question.

    // double vm, rssm;
    // OS::getProcessSizeKB(vm, rssm);
    // vm *=1024; rssm *=1024;
    // LogSevere("Current RAM doing nothing is " << Strings::formatBytes(rssm) << "\n");
    { ProcessTimer fail("MERGE:");

      db.mergeObservations(radarPtr, newSourcePtr, cutoff);
      LogInfo(fail << "\n");
    }


    // clear the marked array
    db.myMarked = nullptr;

    // Dump source list
    db.dumpSources();

    LogInfo("Final size received: " << points << " points, " << missingcounter << " missing.  Total: " << total <<
      "\n");

    //    db.dumpXYZ();

    // If we're realtime we'll get a heartbeat for us, if not we need to process at
    // 'some' point for archive.  We could add more settings/controls for this later
    // This will try to merge and output every incoming record
    if (isArchive()) {
      mergeAndWriteOutput(myLastDataTime, myLastDataTime);
    }
  }
} // RAPIOFusionTwoAlg::processNewData

void
RAPIOFusionTwoAlg::processHeartbeat(const Time& n, const Time& p)
{
  if (isDaemon()) { // just checking, don't think we get message if we're not
    LogInfo("Received heartbeat at " << n << " for event " << p << ".\n");
    mergeAndWriteOutput(n, p);
  }
}

void
RAPIOFusionTwoAlg::mergeAndWriteOutput(const Time& n, const Time& p)
{
  // Note: firstDataSetup might not be called yet...
  // so check database, etc...
  if (myDatabase == nullptr) {
    LogInfo("Haven't received any data yet, so nothing to merge/write.\n");
    return;
  }
  if (myLLGCache == nullptr) {
    LogInfo("We don't have an LLG cache to write to?.\n");
    return;
  }

  // myDatabase->dumpXYZ(); // only valid after firstDataSetup (currently)

  ProcessTimer timepurge("Full time purge. Can still be optimized.");

  // Time to expire data
  // We might not have gotten data in a while..slowly expire off
  // the heartbeat time.  Archive won't have a heartbeat so we'll need
  // some work for that.
  // const Time cutoffTime = myLastDataTime - myMaximumHistory;
  const Time cutoffTime = p - myMaximumHistory;
  const time_t cutoff   = cutoffTime.getSecondsSinceEpoch();

  myDatabase->timePurge(myLastDataTime, myMaximumHistory);
  LogInfo(timepurge << "\n");

  // ----------------
  // Alpha Merge data and write... (fun times)
  // Time, etc. needs to be added
  // ----------------

  // For moment just take everything as an I-frame
  // and use real time
  myLLGCache->fillPrimary(Constants::DataUnavailable);

  // Write out
  myDatabase->mergeTo(myLLGCache, cutoff);

  auto heightsKM = myFullGrid.getHeightsKM();
  // Time aTime = Time::CurrentTime();
  Time aTime = p;

  for (size_t layer = 0; layer < heightsKM.size(); layer++) {
    auto output = myLLGCache->get(layer);
    output->setTime(aTime); // For moment use current real time
    const std::string myWriteCAPPIName = "Fused2" + myTypeName;
    // LogInfo("Writing layer " << layer << " hitCount: " << hitCount[layer] << "\n");
    LogInfo("Writing fused layer " << layer << "\n");
    std::map<std::string, std::string> extraParams;
    extraParams["showfilesize"] = "yes"; // Force compression and sizes for now
    extraParams["compression"]  = "gz";
    writeOutputProduct(myWriteCAPPIName, output, extraParams);
  }
} // RAPIOFusionTwoAlg::processHeartbeat

int
main(int argc, char * argv[])
{
  RAPIOFusionTwoAlg alg = RAPIOFusionTwoAlg();

  alg.executeFromArgs(argc, argv);
} // main
