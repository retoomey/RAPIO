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

  // Debug options
  // Note, normal w2merger writes at a particular time, not per
  // input..so this probably won't keep up.  It's for debugging
  o.boolean("llg", "Turn on/off writing output LatLonGrids per level");
  o.addGroup("llg", "debug");
  o.optional("throttle", "1",
    "Skip count for output files to avoid IO spam during testing, 2 is every other file.  The higher the number, the more files are skipped before writing.");
  o.addGroup("throttle", "debug");

  // Default sync heartbeat to 2 mins
  // Format is seconds then mins
  o.setRequiredValue("sync", "0 */2 * * * *");
}

/** RAPIOAlgorithms process options on start up */
void
RAPIOFusionTwoAlg::processOptions(RAPIOOptions& o)
{
  o.getLegacyGrid(myFullGrid);

  myWriteLLG      = o.getBoolean("llg");
  myThrottleCount = o.getInteger("throttle");
  if (myThrottleCount > 10) {
    myThrottleCount = 10;
  } else if (myThrottleCount < 1) {
    myThrottleCount = 1;
  }
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
  // LogInfo("DATABASE IS " << (void*)(myDatabase.get()) << "\n");
}

void
RAPIOFusionTwoAlg::processNewData(rapio::RAPIOData& d)
{
  // Write out entire llg stack from input field, mostly for
  // testing what we 'have' for the radar matches closely to stage1
  //  bool WRITELLG         = false;
  //  static int writeCount = 0;

  // if (++writeCount >= myThrottleCount) {
  //  if (myWriteLLG) {
  //    WRITELLG = true;
  //  }
  //  writeCount = 0;
  // }

  auto datasp = Stage2Data::receive(d); // Hide internal format in the stage2 data

  size_t missingcounter = 0;
  size_t points         = 0;
  size_t total = 0;

  if (datasp) {
    auto& data = *datasp;
    float v;
    float w;
    short x;
    short y;
    short z;
    std::string name      = data.getRadarName();
    std::string aTypeName = data.getTypeName();
    float elevDegs        = data.getElevationDegs();
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

    #if 0
    if (WRITELLG) {
      // If we're writing out input LLG clones
      // then wipe them to unavailable
      // FIXME: B-frames vs I-frames we'll have to check.  Here
      // we're assuming a full frame of output data.
      // We can't have P-frames at least in real time
      myLLGCache->fillPrimary(Constants::DataUnavailable);
      auto heightsKM = myFullGrid.getHeightsKM();
      for (size_t layer = 0; layer < heightsKM.size(); layer++) {
        auto output = myLLGCache->get(layer);
        output->setTime(data.getTime());
      }
    }
    #endif // if 0

    const size_t xBase = data.getXBase();
    const size_t yBase = data.getYBase();

    auto& db = *myDatabase;

    // Note: This should be a reference or you'll copy
    auto& radar = db.getRadar(name, elevDegs);

    LogInfo("Key back is " << radar.myID << "\n");

    // The time for all the current new observations
    radar.myTime    = data.getTime();
    radar.myExpired = false;

    const Time cutoff = myLastDataTime - myMaximumHistory; // FIXME: add util to time
    if (radar.myTime < cutoff) {
      db.clearObservations(radar);
      radar.myExpired = true;
      LogSevere("Ignoring data for " << radar.myName << " " << radar.myElevDegs << " since it's too old.\n");
      return;
    }

    ProcessTimer timer("Ingest Radar/Tilt");

    // Delete observations for radar (and xyz references)
    db.clearObservations(radar);

    // Important to reserve sizes of incoming data to avoid any std::vector
    // memory movement
    db.reserveSizes(radar, data.getValueCount(), data.getMissingCount());

    // Could store shorts and then do a move forward pass in output
    const time_t t = radar.myTime.getSecondsSinceEpoch();

    // Stream read stage2
    while (data.get(v, w, x, y, z)) { // add time, other stuff
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
        db.addMissing(x, y, z, t); // update mask
        continue;                  // Skip storing missing values
      } else {
        points++;
      }

      // Point cloud
      db.addObservation(radar, v, w, x, y, z);

      // if we're writing LLG layers...then fill in the data value
      // uniquely in the layer
      #if 0
      if (WRITELLG) {
        auto output = myLLGCache->get(z); // FIXME: Cache should/could provide quick references
        if (output) {
          auto& gridtest = output->getFloat2DRef();
          gridtest[y][x] = v;
        }
      }
      #endif
    }
    LogInfo(timer << "\n");

    // Dump radar tree for now
    myDatabase->timePurge(myLastDataTime, myMaximumHistory);
    db.dumpRadars();

    LogInfo("Final size received: " << points << " points, " << missingcounter << " missing.  Total: " << total <<
      "\n");

    #if 0
    if (WRITELLG) {
      auto heightsKM = myFullGrid.getHeightsKM();
      for (size_t layer = 0; layer < heightsKM.size(); layer++) {
        auto output = myLLGCache->get(layer);
        const std::string myWriteCAPPIName = "Fused2" + myTypeName;
        LogInfo("Writing layer " << layer << " hitCount: " << hitCount[layer] << "\n");
        std::map<std::string, std::string> extraParams;
        extraParams["showfilesize"] = "yes"; // Force compression and sizes for now
        extraParams["compression"]  = "gz";
        writeOutputProduct(myWriteCAPPIName, output, extraParams);
      }
    }
    #endif // if 0
  }
} // RAPIOFusionTwoAlg::processNewData

void
RAPIOFusionTwoAlg::processHeartbeat(const Time& n, const Time& p)
{
  LogInfo("Received heartbeat at " << n << " for event " << p << ".\n");

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

  // First time purge attempt (at write)
  // myDatabase->timePurge(myLastDataTime, myMaximumHistory);

  LogInfo("Here goes nothing...we're trying...\n");

  // ----------------
  // Alpha Merge data and write... (fun times)
  // Time, etc. needs to be added
  // ----------------

  // For moment just take everything as an I-frame
  // and use real time
  myLLGCache->fillPrimary(Constants::DataUnavailable);

  // Write out
  myDatabase->mergeTo(myLLGCache);

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
