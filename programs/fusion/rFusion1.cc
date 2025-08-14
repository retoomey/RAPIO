#include "rFusion1.h"
#include "rBinaryTable.h"
#include "rConfigRadarInfo.h"

#include "rRAPIOPlugin.h"

// Value Resolvers
#include "rRobertLinear1Resolver.h"
#include "rLak2DResolver.h"
#include "rLakResolver1.h"
#include "rVelResolver.h"

// Current moving average smoother, prefilter on RadialSets
#include "rLakRadialSmoother.h"

using namespace rapio;

/*
 * New merger stage 1, focusing on specialized on-the-fly caching
 *
 * @author Robert Toomey
 **/
void
RAPIOFusionOneAlg::declarePlugins()
{
  // Declare plugins here, this will allow the help to show a list
  // of the available plugin names

  // -------------------------------------------------------------
  // Elevation Volume registration
  PluginVolume::declare(this, "volume");
  // Volume::introduce("yourvolume", myVolumeClass); To add your own

  // -------------------------------------------------------------
  // VolumeValueResolver plugin registration and creation
  PluginVolumeValueResolver::declare(this, "resolver");
  RobertLinear1Resolver::introduceSelf();
  LakResolver1::introduceSelf();
  VelResolver::introduceSelf();
  Lak2DResolver::introduceSelf();
  // VolumeValueResolver::introduce("yourresolver", myResolverClass); To add your own

  // -------------------------------------------------------------
  // TerrainBlockage plugin registration and creation
  PluginTerrainBlockage::declare(this, "terrain");
  // TerrainBlockage::introduce("yourterrain", myTerrainClass); To add your own

  // Partitioner
  PluginPartition::declare(this, "partition");
}

void
RAPIOFusionOneAlg::declareOptions(RAPIOOptions& o)
{
  o.setDescription(
    "RAPIO Fusion Stage One Algorithm.  Designed to run for a single radar/moment.");
  o.setAuthors("Robert Toomey");

  // legacy use the t, b and s to define a grid
  o.declareLegacyGrid();

  // -----------------------------------------------
  // Flags for outputting for testing/temp that I plan to change at some point.
  // FIXME: Final algorithm this will probably need some rework since we'll
  // be outputting differently for the stage 2 multiradar merging
  o.boolean("subgrid",
    "When on, subgrid any llg output such as netcdf/mrms.  Basically make files using the box around radar vs the full grid.");
  o.addGroup("subgrid", "debug");
  o.optional("throttle", "1",
    "Skip count for output files to avoid IO spam during testing, 2 is every other file.  The higher the number, the more files are skipped before writing.");
  o.addGroup("throttle", "debug");
  o.boolean("presmooth", "Apply Lak's moving average filter to the incoming radial set.");
  o.addGroup("presmooth", "debug");

  // A boolean to set missing to unavailable in output, basically turning off the mask.
  // At this point I consider the background mask/unavailable to be a separate product.
  o.boolean("nomissing",
    "Missing output values become unavailable.");
  o.addAdvancedHelp("nomissing",
    "Resolvers typically calculate values as well as a background mask of missing/unavailable values.  This flag makes any missing output become unavailable, which basically removes the background mask in situations where the data may be unreliable.  It can also speed things up in low weather conditions, but at the cost of loss of background information.");

  // Range to use in KMs.  Default is 460.  This determines subgrid and max range of valid
  // data for the radar
  o.optional("rangekm", "460", "Range in kilometers for radar.");

  // Global weight scale.  Can be used to reduce or increase weight of a group of radars relative to the whole.
  // This works if we manually group radar types say 88d and Climavision.  Eventually this may deprecate to
  // radarinfo.dat and having roster do more advanced decisions on weighting/nearest, or maybe this option will
  // continue to be useful for archive.
  o.optional("weight", "1.0", "Weight percentage multiplier.");
  o.addAdvancedHelp("weight",
    "Percentage to multiply weights by.  This can reduce or increase globally this radar's contribution.  For example, 0.50 means our weights are halved, while 2.0 that radar would double in the final output of stage2.\nNOTE: The resolver has to be coded to use this provided weight or it has no effect.");
  o.addGroup("weight", "weighting");

  o.optional("S", "50",
    "Sigma for exponential distance weighting exp(-(range^2)/variance, where variance = 25*sigma*sigma.");
  o.addAdvancedHelp("S",
    "For legacy reasons, variance = 25*sigma*sigma. So a value of S of 50 yields a variance of 62500, and S of 10 yields 2500 in the denominator the exp weighting.");
  o.addGroup("S", "weighting");

  // Roster means we write coverage files and read masks
  // A missing mask:
  //      When NOT using a roster means nothing, we write everything.
  //      When using a roster means we write nothing (multi radar coop).
  o.optional("roster", "", "Location of roster/cache folder.");
  o.addAdvancedHelp("roster",
    "Using a roster folder means we write coverage files and get back masks from a running roster that tells us what to write, because we are part of a N nearest radar cluster.  This means if a mask is missing we don't write anything.  Without a roster, we always write all our data.");

  // Default sync heartbeat to 30 seconds
  // Format is seconds then mins
  o.setDefaultValue("sync", "*/30 * * * * *");

  // Super verbose and slow.  Output after every processed tilt.  Usually in archive/realtime
  // you output after so much time has passed, either say 2 minutes of realtime or 2 minutes
  // of records
  o.boolean("everytilt", "Output after every tilt received and ignore heartbeat/sync");
  o.addGroup("everytilt", "time");

  // Output S2 by default and declare static product keys for what we write.
  o.setDefaultValue("O", "S2");
  declareProduct("S2", "Write Stage2 raw data files. (Normal operations)");
  declareProduct("2D", "Write 2D layers directly (debugging)");
  declareProduct("S2Netcdf", "Write Stage2 as netcdf (need netcdf output folder in -o) (debugging)");
} // RAPIOFusionOneAlg::declareOptions

/** RAPIOAlgorithms process options on start up */
void
RAPIOFusionOneAlg::processOptions(RAPIOOptions& o)
{
  // ----------------------------------------
  // Check partition information
  bool success = false;
  auto part    = getPlugin<PluginPartition>("partition");

  if (part) {
    o.getLegacyGrid(myFullGrid);
    success = part->getPartitionInfo(myFullGrid, myPartitionInfo);
  }
  if (!success) {
    LogSevere("Failed to load and/or parse partition information!\n");
    exit(1);
  }

  myLLProjections.resize(myFullGrid.getNumZ());
  myLevelSames.resize(myFullGrid.getNumZ());
  if (myLLProjections.size() < 1) {
    LogSevere("No height layers to process, exiting\n");
    exit(1);
  }

  myWriteStage2Name  = "None";          // will get from first radialset typename
  myWriteCAPPIName   = "None";          // will get from first radialset typename
  myWriteOutputUnits = "Dimensionless"; // will get from first radialset units
  myWriteSubgrid     = o.getBoolean("subgrid");
  myUseLakSmoothing  = o.getBoolean("presmooth");
  myThrottleCount    = o.getInteger("throttle");
  if (myThrottleCount > 10) {
    myThrottleCount = 10;
  } else if (myThrottleCount < 1) {
    myThrottleCount = 1;
  }
  myRangeKMs = o.getFloat("rangekm");
  if (myRangeKMs < 50) {
    myRangeKMs = 50;
  } else if (myRangeKMs > 1000) {
    myRangeKMs = 1000;
  }
  LogInfo("Radar range is " << myRangeKMs << " Kilometers.\n");

  // Weight information
  myWeight = o.getFloat("weight");
  bool goodWeight = true;

  if ((myWeight <= 0) || (myWeight > 10.0)) { // Some reasonable ranges?
    LogSevere("Weight given is " << myWeight << " which seems wrong, setting to 1.0\n");
    myWeight = 1.0;
  }

  // Missing or not?
  myNoMissing = o.getBoolean("nomissing");

  mySigmaWeight = o.getFloat("S");
  if (mySigmaWeight <= 0) {
    LogSevere("Sigma given is " << mySigmaWeight << " which seems wrong, setting to 50.\n");
    mySigmaWeight = 50;
  }

  std::string roster = o.getString("roster");

  myUseRoster = !roster.empty();
  if (myUseRoster) {
    FusionCache::setRosterDir(roster);
  }
  myEveryTilt = o.getBoolean("everytilt");
} // RAPIOFusionOneAlg::processOptions

void
RAPIOFusionOneAlg::createLLGCache(
  const std::string    & outputName,
  const std::string    & outputUnits,
  const LLCoverageArea & g)
{
  // NOTE: Tried it as a 3D array.  Due to memory fetching, etc. having N 2D arrays
  // turns out to be faster than 1 3D array.
  if (myLLGCache == nullptr) {
    ProcessTimer("Creating initial LLG value cache.");

    myLLGCache = LLHGridN2D::Create(outputName, outputUnits, Time(), g);
    myLLGCache->fillPrimary(Constants::DataUnavailable); // Create/fill all sublayers to unavailable
  }
}

void
RAPIOFusionOneAlg::createLLHtoAzRangeElevProjection(
  AngleDegs cLat, AngleDegs cLon, LengthKMs cHeight,
  LLCoverageArea& g)
{
  // We cache a bunch of repeated trig functions that save us a lot of CPU time
  if (myLLProjections[0] == nullptr) {
    ProcessTimer projtime("Projection AzRangeElev cache generation.\n");
    // LogInfo("Projection AzRangeElev cache generation.\n");
    LengthKMs virtualRangeKMs;
    AngleDegs virtualAzDegs, virtualElevDegs;

    // Create the sin/cos cache.  We just need one of these vs the Az cache below
    // that is per conus layer
    mySinCosCache = std::make_shared<SinCosLatLonCache>(g.getNumX(), g.getNumY());

    // LogInfo("Projection LatLonHeight to AzRangeElev cache creation...\n");
    auto heightsKM = g.getHeightsKM();
    for (size_t i = 0; i < myLLProjections.size(); i++) {
      // const LengthKMs layerHeightKMs = myHeightsM[i]; //   1000.0;
      const LengthKMs layerHeightKMs = heightsKM[i]; //   1000.0;
      // LogDebug("  Layer: " << heightsKM[i] * 1000.0 << " meters.\n");
      myLLProjections[i] = std::make_shared<AzRanElevCache>(g.getNumX(), g.getNumY());
      // Go ahead and make this cache too
      myLevelSames[i] = std::make_shared<LevelSameCache>(g.getNumX(), g.getNumY());
      auto& llp = *(myLLProjections[i]);
      auto& ssc = *(mySinCosCache);
      AngleDegs startLat = g.getNWLat() - (g.getLatSpacing() / 2.0); // move south (lat decreasing)
      AngleDegs startLon = g.getNWLon() + (g.getLonSpacing() / 2.0); // move east (lon increasing)
      AngleDegs atLat    = startLat;

      llp.reset();
      size_t atssc = 0;

      for (size_t y = 0; y < g.getNumY(); y++, atLat -= g.getLatSpacing()) { // a north to south
        AngleDegs atLon = startLon;
        // a east to west row, changing lon per cell
        for (size_t x = 0; x < g.getNumX(); x++, atLon += g.getLonSpacing(), llp.next()) {
          //
          // The sin/cos attenuation factors from radar center to lat lon
          // This doesn't need height so it can be just a single layer
          double sinGcdIR, cosGcdIR;
          if (i == 0) {
            // First layer we'll create the sin/cos cache
            Project::stationLatLonToTarget(
              atLat, atLon, cLat, cLon, sinGcdIR, cosGcdIR);
            ssc.set(atssc, sinGcdIR, cosGcdIR);
          } else {
            ssc.get(atssc, sinGcdIR, cosGcdIR);
          }

          // Calculate and cache a virtual azimuth, elevation and range for each
          // point of interest in the grid
          Project::Cached_BeamPath_LLHtoAzRangeElev(atLat, atLon, layerHeightKMs,
            cLat, cLon, cHeight, sinGcdIR, cosGcdIR, virtualElevDegs, virtualAzDegs, virtualRangeKMs);

          llp.setAt(virtualAzDegs, virtualElevDegs, virtualRangeKMs);
          atssc++;
        }
      }
    }
    LogInfo(projtime);
  }
} // RAPIOFusionOneAlg::createLLHtoAzRangeElevProjection

void
RAPIOFusionOneAlg::writeOutputCAPPI(std::shared_ptr<LatLonGrid> output)
{
  std::map<std::string, std::string> extraParams;

  extraParams["showfilesize"] = "yes"; // Force compression and sizes for now
  extraParams["compression"]  = "gz";

  if (!myWriteSubgrid) {
    // FIXME: check grids aren't already same size or this is wasted

    // Lazy create a full CONUS output buffer
    // Create a single 2D covering full grid (for outputting full vs subgrid files)
    // The web page, etc. require full conus referenced data.
    // We don't worry about height, we'll set it for each layer
    if (myFullLLG == nullptr) {
      LogInfo("Creating a full CONUS buffer for outputting grids as full files\n");
      myFullLLG = LatLonGrid::Create(myWriteStage2Name, myWriteOutputUnits,
          LLH(myFullGrid.getNWLat(), myFullGrid.getNWLon(), 0), Time(),
          myFullGrid.getLatSpacing(), myFullGrid.getLonSpacing(),
          myFullGrid.getNumY(), myFullGrid.getNumX());
      // Make sure outside our effective area is unavailable
      auto fullgrid = myFullLLG->getFloat2D();
      fullgrid->fill(Constants::DataUnavailable);
    }

    // Copy the current output subgrid into the full LLG for writing out...
    // Note that since the 2D coordinates don't change, any 'outside' part doesn't need
    // to be written and can stay DataUnavailable
    // FIXME: Could maybe be a copy method in LatLonGrid with multiple coverage areas
    auto& fullgrid = myFullLLG->getFloat2DRef();
    auto& subgrid  = output->getFloat2DRef();

    // Sync height and time with the current output grid
    // FIXME: a getHeight/setHeight?  Seems silly to pull/push here so much
    myFullLLG->setTime(output->getTime());
    auto fullLoc = myFullLLG->getLocation();
    auto subLoc  = output->getLocation();
    fullLoc.setHeightKM(subLoc.getHeightKM());
    myFullLLG->setLocation(fullLoc);

    // Copy subgrid into the full grid
    size_t startY = myRadarGrid.getStartY();
    size_t startX = myRadarGrid.getStartX();
    for (size_t y = 0; y < myRadarGrid.getNumY(); y++) {   // a north to south
      for (size_t x = 0; x < myRadarGrid.getNumX(); x++) { // a east to west row
        fullgrid[startY + y][startX + x] = subgrid[y][x];
      }
    }
    // Full grid file (CONUS for regular merger)
    myFullLLG->setTypeName(myWriteCAPPIName);
    writeOutputProduct("2D", myFullLLG, extraParams);
  } else {
    // Just write the subgrid directly (typically smaller than the full grid)
    output->setTypeName(myWriteCAPPIName);
    writeOutputProduct("2D", output, extraParams);
  }
} // RAPIOFusionOneAlg::writeOutputCAPPI

void
RAPIOFusionOneAlg::firstDataSetup(std::shared_ptr<RadialSet> r, const std::string& radarName,
  const std::string& typeName)
{
  static bool setup = false;

  if (setup) { return; }

  setup = true;
  LogInfo(ColorTerm::green() << ColorTerm::bold() << "---Initial Startup---" << ColorTerm::reset() << "\n");

  // Radar center coordinates
  const LLH center        = r->getRadarLocation();
  const AngleDegs cLat    = center.getLatitudeDeg();
  const AngleDegs cLon    = center.getLongitudeDeg();
  const LengthKMs cHeight = center.getHeightKM();

  // Link to first incoming radar and moment, we will ignore any others from now on
  LogInfo(
    "Linking this algorithm to radar '" << radarName << "' and typename '" << typeName <<
      "' since first pass we only handle 1\n");
  myTypeName    = typeName;
  myRadarName   = radarName;
  myRadarCenter = center;

  // We inset the full merger grid to the subgrid covered by the radar.
  // This is needed for massively large grids like the CONUS.
  const LengthKMs fudgeKMs = 5; // Extra to include so circle is inside box a bit

  myRadarGrid = myFullGrid.insetRadarRange(cLat, cLon, myRangeKMs + fudgeKMs);
  LogInfo("Radar center:" << cLat << "," << cLon << " at " << cHeight << " KMs\n");
  LogInfo("Full coverage grid: " << myFullGrid << "\n");
  LLCoverageArea outg = myRadarGrid;

  LogInfo("Radar subgrid: " << outg << "\n");

  std::string key, params;

  // -------------------------------------------------------------
  // VolumeValueResolver creation
  auto vp = getPlugin<PluginVolumeValueResolver>("resolver");

  if (vp) {
    myResolver = vp->getVolumeValueResolver(); // This will exit if not available
    myResolver->setGlobalWeight(myWeight);
    if (myNoMissing) {
      LogInfo("No missing set, turning off missing output values\n");
      myResolver->setMissingValue(Constants::DataUnavailable);
    }

    // 25 here is a magic number from w2merger for time variance squared.
    myResolver->setVarianceWeight(1.0 / (25.0 * (mySigmaWeight * mySigmaWeight)));
  }

  // -------------------------------------------------------------
  // Terrain blockage creation
  auto tp = getPlugin<PluginTerrainBlockage>("terrain");

  if (tp) {
    // This is allowed to be nullptr
    myTerrainBlockage = tp->getNewTerrainBlockage(r->getLocation(), myRangeKMs, radarName);
  }

  // -------------------------------------------------------------
  // Elevation volume creation
  auto volp = getPlugin<PluginVolume>("volume");

  if (volp) {
    myElevationVolume = volp->getNewVolume(myRadarName + "_" + myTypeName);
  }

  // Look up from cells to az/range/elev for RADAR
  // {
  //   ProcessTimer rangeCache("Testing time for range cache..");
  createLLHtoAzRangeElevProjection(cLat, cLon, cHeight, outg);
  //   LogSevere(rangeCache << "\n");
  //   exit(1);
  // }

  // Generate output name and units.
  // FIXME: More control flags, maybe even name options
  myWriteStage2Name  = "Mapped" + r->getTypeName();
  myWriteCAPPIName   = "Fused1" + r->getTypeName();
  myWriteOutputUnits = r->getUnits();
  if (myWriteOutputUnits.empty()) {
    LogSevere("Units still wonky because of W2 bug, forcing dBZ at moment..\n");
    myWriteOutputUnits = "dBZ";
  }

  // Create working LLG cache CAPPI storage per height level
  createLLGCache(myWriteStage2Name, myWriteOutputUnits, outg);
} // RAPIOFusionOneAlg::firstDataSetup

void
RAPIOFusionOneAlg::updateRangeFile()
{
  // If not using roster, we don't write a coverage file for it
  if (!myUseRoster) {
    return;
  }
  // We either have to write/rewrite the range file, or at least touch it.
  // For moment we're rewriting it.  Multi moment they may step on each other,
  // so we'll either pull it out to another program or check time stamps or
  // something.  Have to think about it and run operationally to determine best design.

  // Wait a few tilts so we don't just write a blank coverage file
  static size_t startupcounter = 0;
  static bool writeRange       = false;

  if (!writeRange) {
    if (++startupcounter > 2) { // wait until 3rd tilt
      writeRange = true;
    } else {
      return;
    }
  }

  // Write a roster file
  std::string directory;
  std::string filename = FusionCache::getRangeFilename(myRadarName, myFullGrid, directory);
  std::string fullpath = directory + filename;

  LogInfo("Writing roster file: " << fullpath << "\n");
  // LogInfo("Sizes: " << myRadarGrid.getNumX() << ", " << myRadarGrid.getNumY() << "\n");
  OS::ensureDirectory(directory);
  FusionCache::writeRangeFile(fullpath, myRadarGrid,
    myLLProjections, myRangeKMs);
} // RAPIOFusionOneAlg::updateRangeFile

void
RAPIOFusionOneAlg::readCoverageMask()
{
  // If not using roster, there's no mask, we write everything
  if (!myUseRoster) {
    myHaveMask = false;
    return;
  }

  // Read a mask file
  // if useRoster set..otherwise all points or 1
  std::string directory;
  std::string maskfile = FusionCache::getMaskFilename(myRadarName, myFullGrid, directory);
  std::string fullpath = directory + maskfile;

  OS::ensureDirectory(directory);

  // Bitset mask;
  if (!FusionCache::readMaskFile(fullpath, myMask)) {
    // FIXME: eventually harden this stuff, like if x is wrong, etc.  For now we're just
    // getting it to work
    myMask.clearAllBits(); // Missing mask AND using roster then no output basically...
    myHaveMask = false;
    LogInfo("No mask found at: " << fullpath << "\n");
  } else {
    LogInfo("Mask found at: " << fullpath << "\n");
    myHaveMask = true;

    // Do a quick check on mask dimensions to match us:
    auto d = myMask.getDimensions();
    if ((d.size() > 2) && (myRadarGrid.getNumX() == d[0]) && (myRadarGrid.getNumY() == d[1]) &&
      (myRadarGrid.getNumZ() == d[2]))
    { } else {
      myHaveMask = false;
      if (d.size() < 3) {
        LogSevere("Mask does not have 3 dimensions, can't use it.  Dimensions was " << d.size() << "\n");
      } else {
        LogSevere("Mask does not match our coverage grid, can't use it: " <<
          myRadarGrid.getNumX() << ", " << myRadarGrid.getNumY() << ", " << myRadarGrid.getNumZ() << " != " <<
          d[0] << ", " << d[1] << ", " << d[2] << "\n");
      }
    }
  }
} // RAPIOFusionOneAlg::readCoverageMask

void
RAPIOFusionOneAlg::processRadialSet(std::shared_ptr<RadialSet> r)
{
  LogInfo(ColorTerm::green() << ColorTerm::bold() << "---RadialSet---" << ColorTerm::reset() << "\n");
  // Need a radar name in data to handle it currently
  std::string name = "UNKNOWN";

  if (!r->getString("radarName-value", name)) {
    LogSevere("No radar name found in RadialSet, ignoring data\n");
    return;
  }

  // Get the radar name and typename from this RadialSet.
  const std::string aTypeName = r->getTypeName();
  const std::string aUnits    = r->getUnits();
  ProcessTimer ingest("Ingest tilt");

  LogInfo(
    r->getTypeName() << " (" << r->getNumRadials() << " Radials * " << r->getNumGates() << " Gates), " <<
      r->getElevationDegs() << ", Time: " << r->getTime() << "\n");

  // Initialize everything related to this radar
  firstDataSetup(r, name, aTypeName);

  // Check if incoming radar/moment matches our single setup, otherwise we'd need
  // all the setup for each radar/moment.  Which we 'could' do later maybe
  if ((myRadarName != name) || (myTypeName != aTypeName)) {
    LogSevere(
      "We are linked to '" << myRadarName << "-" << myTypeName << "', ignoring radar/typename '" << name << "-" << aTypeName <<
        "'\n");
    return;
  }

  // Update the range/coverage file for us so Roster knows we're active radar
  // and includes us into the mask calculations
  updateRangeFile();

  // Smoothing calculation.  Interesting that w2merger for 250 meter and 1000 meter conus
  // is calcuating 3 gates not 4 like in paper.  Suspecting a bug.
  // FIXME: If we plugin the smoother we can pass params and choose the scale factor
  // manually.
  const LengthKMs radarDataScale = r->getGateWidthKMs();
  const LengthKMs gridScale      = std::min(myFullGrid.getLatKMPerPixel(), myFullGrid.getLonKMPerPixel());
  const int scale_factor         = int (0.5 + gridScale / radarDataScale); // Number of gates

  if ((scale_factor > 1) && myUseLakSmoothing) {
    LogInfo("Applying Lak's moving average smoothing filter to RadialSet, " << scale_factor <<
      " filter ratio scale.\n");
    LakRadialSmoother::smooth(r, scale_factor / 2);
  } else {
    LogInfo("Not applying smoothing since scale factor is " << scale_factor << "\n");
  }

  // Assign the ID key for cache storage.  Note the size matters iff you have more
  // DataTypes to keep track of than the key size.  Currently FusionKey defines the key
  // size and max unique elevations we can hold at once.
  static FusionKey keycounter = 0; // 0 is reserved for nothing and not used

  if (++keycounter == 0) { keycounter = 1; } // skip 0 again on overflow
  r->setID(keycounter);
  // LogDebug("RadialSet short key: " << (int) keycounter << "\n");

  // Always add to elevation volume
  myElevationVolume->addDataType(r);

  // ----------------------------------------------------------------------------
  // Every Unique RadialSet product will require a RadialSetLookup.
  // This is a projection from range, angle to gate/radial index.
  r->getProjection();

  // Every RadialSet will require terrain per gate for filters.
  // Run a polar terrain algorithm where the results are added to the RadialSet as
  // another array.
  if (myTerrainBlockage != nullptr) {
    ProcessTimer terrain("Applying terrain blockage\n");
    myTerrainBlockage->calculateTerrainPerGate(r);
    LogInfo(terrain);
  }
} // RAPIOFusionOneAlg::processRadialSet

void
RAPIOFusionOneAlg::processNewData(rapio::RAPIOData& d)
{
  // Look for any data the system knows how to read...
  // auto r = d.datatype<rapio::DataType>();
  auto r = d.datatype<rapio::RadialSet>();

  if (r != nullptr) {
    processRadialSet(r);

    // Per incoming tilt triggering.  Issue here is if we get a 'burst'
    // say with a full Canada volume then we do a silly amount of
    // duplicate processing since tilts overlap in effective output.
    // So instead, we'll try waiting for a heartbeat...
    // FIXME: note that archive won't work properly.  I'm really leaning
    // towards removal of realtime flags and doing the simulation technique
    // const Time rTime   = r->getTime();
    // processVolume(rTime);
    myDirty++;
  }
  // Output now on the every tilt option.  We're ignoring heartbeats
  if (myEveryTilt) {
    processVolume(r->getTime());
  }
}

void
RAPIOFusionOneAlg::processHeartbeat(const Time& n, const Time& p)
{
  // Realtime/archive sends us pulses.  But if we're outputting every tilt
  // no need for doing it here.
  if (!myEveryTilt) {
    LogInfo(ColorTerm::green() << ColorTerm::bold() << "---Heartbeat---" << ColorTerm::reset() << "\n");
    processVolume(p); // n or p is the question...?
  }
}

size_t
RAPIOFusionOneAlg::processHeightLayer(size_t layer,
  const VolumePointerCache& cc,
  const Time                                 & rTime, // Time for writing a 2D debug layer
  std::shared_ptr<VolumeValueIO>             stage2p)
{
  // Prepping for threading layers here.  Since each layer is independent
  // in memory we 'should' be able to thread them with minimum locking.  This
  // will give us more CPU scaling.
  // FIXME: check reads/writes here thread safe and add any needed locking

  // ------------------------------------------------------------------------------
  // Do we try to output Stage 2 files for fusion2?
  const bool outputStage2 = (isProductWanted("S2") || isProductWanted("S2Netcdf"));

  // Grid information
  const LLCoverageArea& outg = myRadarGrid;
  const auto heightsKM       = outg.getHeightsKM();
  // Note: The 'range' is 0 to numY always, however the index to global grid is y+out.startY;
  // Note: The 'range' is 0 to numX always, however the index to global grid is x+out.startX;
  const size_t numY = outg.getNumY();
  const size_t numX = outg.getNumX();

  // Set the value object for resolver
  auto& resolver = *myResolver;
  std::shared_ptr<VolumeValue> vvsp = resolver.getVolumeValue();
  auto& vv   = *vvsp;
  auto * vvp = vvsp.get();

  vv.setRadarLocation(myRadarCenter);
  vv.setAtLocationHeightKMs(heightsKM[layer]);

  auto& ev = *myElevationVolume;

  // Each 2D layer is independent so threading here should be ok
  auto output    = myLLGCache->get(layer);
  auto& gridtest = output->getFloat2DRef();

  /** Get cache start pointers */
  auto& ssc = *(mySinCosCache);
  const double * sinGcdIR = ssc.sinGcdIRData();
  const double * cosGcdIR = ssc.cosGcdIRData();

  auto& llp       = *myLLProjections[layer];
  auto& levelSame = *myLevelSames[layer];

  llp.reset();
  levelSame.reset();

  // Stats
  const size_t totalLayer = numX * numY; // Total layer points
  size_t maskSkipped      = 0;           // Skip by mask (roster nearest)
  size_t rangeSkipped     = 0;           // Skip by range (outside circle)
  size_t sameTiltSkip     = 0;           // Enclosing tilts did not change
  size_t attemptCount     = 0;           // Resolver calculation

  // Operations needs to be the fastest, so in this special case we check
  // all flags outside the loop:
  // Upside is speed, downside is two copies of code here to keep in sync
  const bool writeLLG = isProductWanted("2D");

  AngleDegs startLat = outg.getNWLat() - (outg.getLatSpacing() / 2.0); // move south (lat decreasing)
  AngleDegs startLon = outg.getNWLon() + (outg.getLonSpacing() / 2.0); // move east (lon increasing)
  AngleDegs atLat    = startLat;

  // Partitioning vectors...tracking which partition we are in
  // Get our starting base partition indexes in X,Y
  const size_t sX = outg.getStartX();
  const size_t sY = outg.getStartY();
  const size_t startPartXDim = myPartitionInfo.getXDimFor(sX);
  const size_t startPartYDim = myPartitionInfo.getYDimFor(sY);

  // --------------------------------------------
  // Operations special case.
  // We remove a lot of ifs from inside the loop
  // to give a slight speed boost.
  // Bad side is duplication between our operations loop and the general loop below
  // FIXME: template or constexpr maybe we can logically unify this code.
  // FIXME: another option is stage1 gets 'fast enough' and we just take the flag check hit
  //
  if (stage2p && myHaveMask && outputStage2 && !writeLLG) {
    auto& stage2 = *stage2p;

    size_t yPartDim = startPartYDim;
    for (size_t y = 0; y < numY; ++y, atLat -= outg.getLatSpacing()) {
      myPartitionInfo.nextY(sY + y, yPartDim); // increment dimension Y if needed
      AngleDegs atLon = startLon;
      size_t xPartDim = startPartXDim;
      for (size_t x = 0; x < numX; ++x, atLon += outg.getLonSpacing(), llp.next(),
        sinGcdIR++, cosGcdIR++, levelSame.next())
      {
        myPartitionInfo.nextX(sX + x, xPartDim); // increment dimension X if needed

        // Mask check.
        size_t mi = myMask.getIndex3D(x, y, layer); // FIXME: Maybe we can be linear here
        if (!myMask.get1(mi)) {
          maskSkipped++;
          continue;
        }

        // Range check.
        llp.getRangeKMsAt(vv.virtualRangeKMs);
        if (vv.virtualRangeKMs > myRangeKMs) {
          rangeSkipped++;
          continue;
        }

        // Search the virtual volume for elevations above/below our virtual one
        llp.getElevDegsAt(vv.virtualElevDegs);
        ev.getSpreadNew(cc, vv);

        // If the upper and lower for this point are the _same_ RadialSets as before, then
        // the interpolation will end up with the same data value.
        // This takes advantage that in a volume if a couple tilts change, then
        // the space time affected is fairly small compared to the entire volume's 3D coverage.
        const bool changedEnclosingTilts = levelSame.setAt(vv.getLower(), vv.getUpper(),
            vv.get2ndLower(), vv.get2ndUpper());
        if (!changedEnclosingTilts) { // The value won't change, so continue
          sameTiltSkip++;
          continue;
        }

        // Resolve the value/weight at location and add to output
        attemptCount++;
        vv.sinGcdIR = *sinGcdIR;
        vv.cosGcdIR = *cosGcdIR;
        llp.getAzDegsAt(vv.virtualAzDegs);
        vv.setAtLocationLatLonDegs(atLat, atLon);
        resolver.calc(vvp);

        // Probably 'slow' here...calculate index from partition indexes
        // FIXME: Could probably pull out the multiply/add here
        // size_t width = myDims[0]; -> Number of X (const)
        // return y * width + x;  -> So y +width each y, ++ for each X
        size_t partIndex = myPartitionInfo.index(xPartDim, yPartDim);
        stage2.add(vvp, x, y, layer, partIndex);
      } // endX

      // If the part Y index goes over boundary, move to next Y partition index...
    } // endY
  } else {
    // --------------------------------------------
    // General case.
    // More checks for different cases, which will be slightly slower
    //
    size_t yPartDim = startPartYDim;
    for (size_t y = 0; y < numY; ++y, atLat -= outg.getLatSpacing()) {
      myPartitionInfo.nextY(sY + y, yPartDim); // increment dimension Y if needed
      AngleDegs atLon = startLon;
      size_t xPartDim = startPartXDim;
      for (size_t x = 0; x < numX; ++x, atLon += outg.getLonSpacing(), llp.next(),
        sinGcdIR++, cosGcdIR++, levelSame.next())
      {
        myPartitionInfo.nextX(sX + x, xPartDim); // increment dimension X if needed

        // Mask check.
        if (myHaveMask) {
          size_t mi = myMask.getIndex3D(x, y, layer);
          if (!myMask.get1(mi)) {
            gridtest[y][x] = 50;
            maskSkipped++;
            continue;
          }
        }

        // Create lat lon grid of a particular field...
        // gridtest[y][x] = vv.virtualRangeKMs; continue; // Range circles
        // gridtest[y][x] = vv.virtualAzDegs; continue;   // Azimuth

        // Range check.
        llp.getRangeKMsAt(vv.virtualRangeKMs);
        if (vv.virtualRangeKMs > myRangeKMs) {
          // Since this is outside range it should never be different from the initialization
          // gridtest[y][x] = Constants::DataUnavailable;
          rangeSkipped++;
          continue;
        }

        // Search the virtual volume for elevations above/below our virtual one
        // Linear for 'small' N of elevation volume is faster than binary here.  Interesting
        llp.getElevDegsAt(vv.virtualElevDegs);
        ev.getSpreadNew(cc, vv);

        // If the upper and lower for this point are the _same_ RadialSets as before, then
        // the interpolation will end up with the same data value.
        // This takes advantage that in a volume if a couple tilts change, then
        // the space time affected is fairly small compared to the entire volume's 3D coverage.
        const bool changedEnclosingTilts = levelSame.setAt(vv.getLower(), vv.getUpper(),
            vv.get2ndLower(), vv.get2ndUpper());
        if (!changedEnclosingTilts) { // The value won't change, so continue
          sameTiltSkip++;
          continue;
        }

        attemptCount++;
        vv.sinGcdIR = *sinGcdIR;
        vv.cosGcdIR = *cosGcdIR;
        llp.getAzDegsAt(vv.virtualAzDegs);
        vv.setAtLocationLatLonDegs(atLat, atLon);
        resolver.calc(vvp);

        gridtest[y][x] = vv.dataValue;

        if (outputStage2 && stage2p) {
          // Probably 'slow' here...calculate index from partition indexes
          // FIXME: Could probably pull out the multiply/add here
          // size_t width = myDims[0]; -> Number of X (const)
          // return y * width + x;  -> So y +width each y, ++ for each X
          size_t partIndex = myPartitionInfo.index(xPartDim, yPartDim);
          stage2p->add(vvp, x, y, layer, partIndex);
        }
      } // endX
    }   // endY
  } // end else

  // --------------------------------------------------------
  // Write debugging 2D LatLonGrid for layer (post entire layer processing)
  //
  if (writeLLG) {
    static int writeCount = 0;
    if (++writeCount >= myThrottleCount) {
      output->setTime(rTime);
      writeOutputCAPPI(output);
    }
    writeCount = 0;
  }

  // --------------------------------------------------------
  // Log info on the layer calculations
  //
  const double percentAttempt = (double) (attemptCount) / (double) (totalLayer) * 100.0;

  LogDebug("KM: " << vv.getAtLocationHeightKMs()
                  << " Mask: " << maskSkipped
                  << " Range: " << rangeSkipped
                  << " Same: " << sameTiltSkip
                  << " Resolved: " << attemptCount << " (" << percentAttempt << "%) of " << totalLayer << ".\n");

  return attemptCount;
} // RAPIOFusionOneAlg::processHeightLayer

void
RAPIOFusionOneAlg::processVolume(const Time rTime)
{
  // Don't do anything unless we got some new data since last time
  if (myDirty < 1) { return; }

  // Read the coverage mask (in real time it could change due to radars up/down)
  readCoverageMask();

  // Don't do anything if we're using a roster and don't have a mask yet...
  // The mask tells us our part of the global, we don't want to flood things,
  // we're cooperating with other radars.
  if (myUseRoster && !myHaveMask) {
    LogInfo("Using roster but no mask, skipping output until we get one.\n");
    return;
  }

  LogInfo("Processing full volume, " << myDirty << " file(s) have been received.\n");
  myDirty = 0;

  // Get the elevation volume pointers and levels for speed
  VolumePointerCache cc;

  // This is a pointer cache for speed in the loop.
  myElevationVolume->getTempPointerVector(cc);
  LogInfo(*myElevationVolume << "\n");

  // Keep stage 2 output code separate, cheap to make this if we don't use it
  LLCoverageArea& outg = myRadarGrid;
  auto heightsKM       = outg.getHeightsKM();
  size_t totalLayer    = outg.getNumX() * outg.getNumY() * outg.getNumZ();

  // Resolver should tell us how to write
  auto stage2IO = myResolver->getVolumeValueIO();

  if (stage2IO) {
    stage2IO->initForSend(myRadarName, myTypeName, myWriteOutputUnits, myRadarCenter, myPartitionInfo, outg);
  }

  size_t attemptCount = 0;

  // Handle all layers
  for (size_t layer = 0; layer < heightsKM.size(); layer++) {
    attemptCount += processHeightLayer(layer, cc, rTime, stage2IO);
  }

  const double percentAttempt = (double) (attemptCount) / (double) (totalLayer) * 100.0;

  LogInfo("Total all layer grid points: " << totalLayer << " (" << percentAttempt << "%).\n");

  // Send stage2 data (note this is a full conus volume)
  if (isProductWanted("S2") || isProductWanted("S2Netcdf")) {
    if (stage2IO) {
      LogInfo("Sending stage2 data: " << myWriteStage2Name << " at " << rTime << "\n");
      stage2IO->send(this, rTime, myWriteStage2Name);
    }
  }
} // RAPIOFusionOneAlg::processNewData

int
main(int argc, char * argv[])
{
  // Create and run fusion stage 1
  RAPIOFusionOneAlg alg = RAPIOFusionOneAlg();

  alg.executeFromArgs(argc, argv);
}
