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
RAPIOFusionTwoAlg::declarePlugins()
{
  // Partitioner
  PluginPartition::declare(this, "partition");
}

void
RAPIOFusionTwoAlg::declareOptions(RAPIOOptions& o)
{
  o.setDescription(
    "RAPIO Fusion Stage Two Algorithm.  Gathers data from multiple radars for mergin values.  Output options are 2D, 3D and Points.");
  o.setAuthors("Robert Toomey");

  // legacy use the t, b and s to define a grid
  o.declareLegacyGrid();

  // Default sync heartbeat to 2 mins
  // Format is seconds then mins
  o.setDefaultValue("sync", "0 */2 * * * *");

  // Output 2D by default and declare static product keys for what we write.
  o.setDefaultValue("O", "2D");
  declareProduct("2D", "Write N 2D layers merged values");
  declareProduct("2DMax", "Write N 2D layers max values");
  declareProduct("3D", "Write a 3D layer");
  declareProduct("3DMax", "Write a 3D max layer");
  declareProduct("S2", "Write Stage2 raw data files");
}

/** RAPIOAlgorithms process options on start up */
void
RAPIOFusionTwoAlg::processOptions(RAPIOOptions& o)
{
  // ----------------------------------------
  // Check partition information
  bool success = false;
  auto part    = getPlugin<PluginPartition>("partition");

  if (part) {
    o.getLegacyGrid(myFullGrid);
    success = part->getPartitionInfo(myFullGrid, myPartitionInfo);
    if (success) {
      // Note: 'none' returns 1 since there is a single global partition basically
      if (myPartitionInfo.myPartitionNumber < 1) {
        LogSevere("Partition selection required, for example tile:2x2:1 where 1 is nw corner.\n");
        exit(1);
      }
    }
  }
  if (!success) {
    LogSevere("Failed to load and/or parse partition information!\n");
    exit(1);
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
    ProcessTimer cache("Creating initial LLG value cache:\n");

    myLLGCache = LLHGridN2D::Create(outputName, outputUnits, Time(), g);
    myLLGCache->fillPrimary(Constants::DataUnavailable);

    for (size_t z = 0; z < g.getNumZ(); z++) {
      std::shared_ptr<LatLonGrid> output = myLLGCache->get(z);

      // Create a secondary float buffer for accumulating weights.  Crazy idea
      auto w = output->addFloat2D("weights", "Dimensionless", { 0, 1 });
      w->fill(0);

      // Create a buffer for accumulating mask.  Tried playing with v/w to do this,
      // but it's just better separate
      // auto m = output->addByte2D("masks", "Dimensionless", { 0, 1 });
      // m->fill(0);
    }

    LogInfo(cache);
  }
}

void
RAPIOFusionTwoAlg::firstDataSetup(std::shared_ptr<Stage2Data> data)
{
  static bool setup = false;

  if (setup) { return; }

  setup = true;
  LogInfo(ColorTerm::fGreen << ColorTerm::fBold << "---Initial Startup---" << ColorTerm::fNormal << "\n");

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
  // This is clipped to the partition we are using.
  // createLLGCache(myWriteCAPPIName, myWriteOutputUnits, myFullGrid);
  createLLGCache(myWriteCAPPIName, myWriteOutputUnits, myPartitionInfo.getSelectedPartition());

  // Finally, create the point cloud database with N observations per point
  myDatabase = std::make_shared<FusionDatabase>(myFullGrid.getNumX(), myFullGrid.getNumY(), myFullGrid.getNumZ());
  LogInfo(
    "Created XYZ array of " << myFullGrid.getNumX() << "*" << myFullGrid.getNumY() << "*" << myFullGrid.getNumZ() <<
      " size\n");
  // LogInfo("DATABASE IS " << (void*)(myDatabase.get()) << "\n");
} // RAPIOFusionTwoAlg::firstDataSetup

void
RAPIOFusionTwoAlg::processNewData(rapio::RAPIOData& d)
{
  LogInfo(ColorTerm::fGreen << ColorTerm::fBold << "---Stage2---" << ColorTerm::fNormal << "\n");
  // We'll check the record time before reading all the data.  This should be >= all the times
  // in the stage2, so we can pretoss out data _before_ reading it in (which is slower)
  // This means we're falling behind somewhat.
  // Quick toss/expire incoming data, anything > cutoff time is too old
  // The global time for all the current new observations. Note we batch observations,
  // so each point can have a unique time.
  // FIXME: I'm not updating radar.myTime here...maybe we're ok?
  // FIXME: Couldn't the record filter do the history cutoff?
  // FIXME: I'm concerned about stage1 grouping things like Canada into a single stage1 data file,
  // since we're only keeping one time per file.  For example, a volume might have times per tilt, yet
  // we're considering it 'new' to a single time.  It's complicated.
  auto record  = d.record();
  auto recTime = record.getTime();
  const Time cutoffTime = myLastHistoryTime - myMaximumHistory; // FIXME: add util to time
  const time_t cutoff   = cutoffTime.getSecondsSinceEpoch();

  if (recTime < cutoffTime) {
    LogSevere("Ignoring OLD record: (" << recTime.getString("%H:%M:%S") << ") " << d.getDescription() << "\n");
    return;
  }
  // If the queue is high we're falling behind probably.
  size_t aSize = Record::theRecordQueue->size(); // FIXME: probably hide the ->

  if (aSize > 30) {
    LogSevere("We have " << aSize << " unprocessed records, probably lagging...\n");
  }

  ProcessTimer reading("Reading I/O file:\n");
  std::shared_ptr<Stage2Data> datasp;

  try{
    datasp = Stage2Data::receive(d); // Hide internal format in the stage2 data
  }catch (const std::exception& e) {
    LogSevere("Error receiving data: " << e.what() << ", ignoring!\n");
    exit(1);
    return;
  }

  LogInfo(reading);

  if (datasp) {
    auto& data            = *datasp;
    std::string name      = data.getRadarName();
    std::string aTypeName = data.getTypeName();
    Time dataTime         = data.getTime();

    LogInfo("Incoming stage2 data for " << name << " " << aTypeName << "\n");

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

    auto& db = *myDatabase;

    #if 0
    const Time cutoffTime = myLastHistoryTime - myMaximumHistory; // FIXME: add util to time
    const time_t cutoff   = cutoffTime.getSecondsSinceEpoch();
    if (radar.myTime < cutoffTime) {
      LogSevere("Ignoring " << radar.myName << " OLD: " << radar.myTime.getString("%H:%M:%S") << "\n");
      return;
    }
    #endif

    ProcessTimer timer("Ingest Source");
    size_t missingcounter = 0;
    size_t points         = 0;
    size_t total = 0;
    db.ingestNewData(data, cutoff, missingcounter, points, total);

    // Dump source list
    db.dumpSources();

    LogInfo("Final size received: " << points << " points, " << missingcounter << " missing.  Total: " << total <<
      "\n");

    //    db.dumpXYZ();
    myDirty++;

    // If we're realtime we'll get a heartbeat for us, if not we need to process at
    // 'some' point for archive.  We could add more settings/controls for this later
    // This will try to merge and output every incoming record
    if (isArchive()) {
      mergeAndWriteOutput(myLastHistoryTime, myLastHistoryTime);
    }
  }
} // RAPIOFusionTwoAlg::processNewData

void
RAPIOFusionTwoAlg::processHeartbeat(const Time& n, const Time& p)
{
  if (isDaemon()) { // just checking, don't think we get message if we're not
    // LogInfo("Received heartbeat at " << n << " for event " << p << ".\n");
    // Trying a slight throttle. Wait for 20 stage2 before writing
    LogInfo(
      ColorTerm::fGreen << ColorTerm::fBold << "---Heartbeat---" << ColorTerm::fNormal << " (" << myDirty <<
        " ingested since last)\n");
    mergeAndWriteOutput(n, p);
    myDirty = 0;
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
  // const Time cutoffTime = myLastHistoryTime - myMaximumHistory;
  const Time cutoffTime = p - myMaximumHistory;
  const time_t cutoff   = cutoffTime.getSecondsSinceEpoch();

  // LogSevere("Cut off epoch: " << cutoff << " vs NOW " << Time::CurrentTime().getSecondsSinceEpoch() << "\n");
  myDatabase->timePurge(myLastHistoryTime, myMaximumHistory);
  LogInfo(timepurge << "\n");

  // ----------------
  // Alpha Merge data and write... (fun times)
  // Time, etc. needs to be added
  // ----------------

  // Write out
  auto& part = myPartitionInfo.getSelectedPartition();

  Time outputTime = p;

  // Extra params (might be better in config)
  std::map<std::string, std::string> extraParams;

  extraParams["showfilesize"] = "yes"; // Force compression and sizes for now
  extraParams["compression"]  = "gz";

  bool wantMerge = (isProductWanted("2D") || isProductWanted("3D"));

  if (wantMerge) {
    myDatabase->mergeTo(myLLGCache, cutoff, part.getStartX(), part.getStartY());
  }

  // ---------------------------------------
  // Output 2D
  std::string name;

  if (isProductWanted("2D")) { // -O="2D", -O="2D=NameWanted
    auto heightsKM = myFullGrid.getHeightsKM();
    for (size_t layer = 0; layer < heightsKM.size(); layer++) {
      LogInfo("Writing fused layer " << layer << "\n");

      // Use current time for layer
      auto output = myLLGCache->get(layer);
      output->setTime(outputTime);
      output->setTypeName("Fused2" + myTypeName);

      extraParams["onewriter"] = "netcdf";
      writeOutputProduct("2D", output, extraParams);
      extraParams["onewriter"] = "";
    }
  }

  // ---------------------------------------
  // Output 3D cube
  if (isProductWanted("3D")) { // -O="3D", -O="3D=NameWanted
    myLLGCache->setTime(outputTime);
    myLLGCache->setTypeName("Fused2" + myTypeName);
    // You must have a netcdf=/folder in your -o when forcing a writer
    extraParams["onewriter"] = "netcdf";
    writeOutputProduct("3D", myLLGCache, extraParams);
    extraParams["onewriter"] = "";
  }

  bool wantMax = (isProductWanted("2DMax") || isProductWanted("3DMax"));

  if (wantMax) {
    myDatabase->maxTo(myLLGCache, cutoff, part.getStartX(), part.getStartY());
  }

  // ---------------------------------------
  // Output 2D Max
  if (isProductWanted("2DMax")) {
    auto heightsKM = myFullGrid.getHeightsKM();
    for (size_t layer = 0; layer < heightsKM.size(); layer++) {
      LogInfo("Writing max layer " << layer << "\n");

      // Use current time for layer
      auto output = myLLGCache->get(layer);
      output->setTime(outputTime);
      output->setTypeName("Fused2Max" + myTypeName);

      extraParams["onewriter"] = "netcdf";
      writeOutputProduct("2D", output, extraParams);
      extraParams["onewriter"] = "";
    }
  }

  // ---------------------------------------
  // Output 3D Max
  if (isProductWanted("3DMax")) {
    myLLGCache->setTime(outputTime);
    myLLGCache->setTypeName("Fused2Max" + myTypeName);
    // You must have a netcdf=/folder in your -o when forcing a writer
    extraParams["onewriter"] = "netcdf";
    writeOutputProduct("3D", myLLGCache, extraParams);
    extraParams["onewriter"] = "";
  }

  // ---------------------------------------
  // Output stage2 again with group
  // -O="S2=GROUP1" I'm thinking here..
  if (isProductWanted("S2")) { // -O="S2"
    LogInfo("Can't write stage2 data yet...but we will soon...");
  }
} // RAPIOFusionTwoAlg::processHeartbeat

int
main(int argc, char * argv[])
{
  // Use the stream ability of binary table to avoid copying on read
  FusionBinaryTable::myStreamRead = true;
  RAPIOFusionTwoAlg alg = RAPIOFusionTwoAlg();

  alg.executeFromArgs(argc, argv);
} // main
