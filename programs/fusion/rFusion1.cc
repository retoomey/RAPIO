#include "rFusion1.h"
#include "rBinaryTable.h"
#include "rConfigRadarInfo.h"
#include "rStage2Data.h"

#include "rRAPIOPlugin.h"

// Value Resolvers
#include "rRobertLinear1Resolver.h"
#include "rLakResolver1.h"

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
  // VolumeValueResolver::introduce("yourresolver", myResolverClass); To add your own

  // -------------------------------------------------------------
  // TerrainBlockage plugin registration and creation
  PluginTerrainBlockage::declare(this, "terrain");
  // TerrainBlockage::introduce("yourterrain", myTerrainClass); To add your own
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
  o.boolean("llg", "Turn on/off writing output LatLonGrids per level");
  o.addGroup("llg", "debug");
  o.boolean("subgrid",
    "When on, subgrid any llg output such as netcdf/mrms.  Basically make files using the box around radar vs the full grid.");
  o.addGroup("subgrid", "debug");
  o.optional("throttle", "1",
    "Skip count for output files to avoid IO spam during testing, 2 is every other file.  The higher the number, the more files are skipped before writing.");
  o.addGroup("throttle", "debug");
  o.boolean("presmooth", "Apply Lak's moving average filter to the incoming radial set.");
  o.addGroup("presmooth", "debug");

  // Range to use in KMs.  Default is 460.  This determines subgrid and max range of valid
  // data for the radar
  o.optional("rangekm", "460", "Range in kilometers for radar.");

  // Set roster directory to home by default for the moment.
  // We might want to turn it off in archive mode or testing at some point
  const std::string home(OS::getEnvVar("HOME"));

  o.optional("roster", home, "Location of roster/cache folder.");

  // Default sync heartbeat to 30 seconds
  // Format is seconds then mins
  o.setRequiredValue("sync", "*/30 * * * * *");
} // RAPIOFusionOneAlg::declareOptions

/** RAPIOAlgorithms process options on start up */
void
RAPIOFusionOneAlg::processOptions(RAPIOOptions& o)
{
  // Get grid
  o.getLegacyGrid(myFullGrid);

  myLLProjections.resize(myFullGrid.getNumZ());
  if (myLLProjections.size() < 1) {
    LogSevere("No height layers to process, exiting\n");
    exit(1);
  }

  myWriteStage2Name  = "None";          // will get from first radialset typename
  myWriteCAPPIName   = "None";          // will get from first radialset typename
  myWriteOutputUnits = "Dimensionless"; // will get from first radialset units
  myWriteLLG         = o.getBoolean("llg");
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
  FusionCache::setRosterDir(o.getString("roster"));

  LogInfo("Radar range is " << myRangeKMs << " Kilometers.\n");
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
      auto& llp = *(myLLProjections[i]);
      auto& ssc = *(mySinCosCache);
      AngleDegs startLat = g.getNWLat() - (g.getLatSpacing() / 2.0); // move south (lat decreasing)
      AngleDegs startLon = g.getNWLon() + (g.getLonSpacing() / 2.0); // move east (lon increasing)
      AngleDegs atLat    = startLat;

      llp.reset();
      ssc.reset();

      for (size_t y = 0; y < g.getNumY(); y++, atLat -= g.getLatSpacing()) { // a north to south
        AngleDegs atLon = startLon;
        for (size_t x = 0; x < g.getNumX(); x++, atLon += g.getLonSpacing()) { // a east to west row, changing lon per cell
          //
          // The sin/cos attenuation factors from radar center to lat lon
          // This doesn't need height so it can be just a single layer
          double sinGcdIR, cosGcdIR;
          if (i == 0) {
            // First layer we'll create the sin/cos cache
            Project::stationLatLonToTarget(
              atLat, atLon, cLat, cLon, sinGcdIR, cosGcdIR);
            ssc.add(sinGcdIR, cosGcdIR); // move forward
          } else {
            ssc.get(sinGcdIR, cosGcdIR); // move forward
          }

          // Calculate and cache a virtual azimuth, elevation and range for each
          // point of interest in the grid
          Project::Cached_BeamPath_LLHtoAzRangeElev(atLat, atLon, layerHeightKMs,
            cLat, cLon, cHeight, sinGcdIR, cosGcdIR, virtualElevDegs, virtualAzDegs, virtualRangeKMs);

          llp.add(virtualAzDegs, virtualElevDegs, virtualRangeKMs);
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
    writeOutputProduct(myWriteCAPPIName, myFullLLG, extraParams);
  } else {
    // Just write the subgrid directly (typically smaller than the full grid)
    writeOutputProduct(myWriteCAPPIName, output, extraParams);
  }
} // RAPIOFusionOneAlg::writeOutputCAPPI

void
RAPIOFusionOneAlg::firstDataSetup(std::shared_ptr<RadialSet> r, const std::string& radarName,
  const std::string& typeName)
{
  static bool setup = false;

  if (setup) { return; }

  setup = true;
  LogInfo(ColorTerm::fGreen << ColorTerm::fBold << "---Initial Startup---" << ColorTerm::fNormal << "\n");

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

  // FIXME: These have to be done repeatedly if we use it as a heartbeat to
  // an active fusion roster. Right now we're just statically running roster

  // Write a roster file
  std::string directory;
  std::string filename = FusionCache::getRangeFilename(myRadarName, myFullGrid, directory);
  std::string fullpath = directory + filename;

  LogInfo("Writing roster file: " << fullpath << "\n");
  LogInfo("Sizes: " << myRadarGrid.getNumX() << ", " << myRadarGrid.getNumY() << "\n");
  OS::ensureDirectory(directory);
  FusionCache::writeRangeFile(fullpath, myRadarGrid,
    myLLProjections, myRangeKMs);
} // RAPIOFusionOneAlg::firstDataSetup

void
RAPIOFusionOneAlg::readCoverageMask()
{
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
  } else {
    LogInfo("Found and read nearest coverage mask: " << fullpath << "\n");
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
  // myHaveMask = false;
} // RAPIOFusionOneAlg::readCoverageMask

void
RAPIOFusionOneAlg::processRadialSet(std::shared_ptr<RadialSet> r)
{
  LogInfo(ColorTerm::fGreen << ColorTerm::fBold << "---RadialSet---" << ColorTerm::fNormal << "\n");
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
}

void
RAPIOFusionOneAlg::processHeartbeat(const Time& n, const Time& p)
{
  LogInfo(ColorTerm::fGreen << ColorTerm::fBold << "---Heartbeat---" << ColorTerm::fNormal << "\n");
  processVolume(p); // n or p is the question...?
}

size_t
RAPIOFusionOneAlg::processHeightLayer(size_t layer,
  const std::vector<double>                  levels, // subtypes/elevation angles
  const std::vector<DataType *>              pointers,
  const Time                                 & rTime,  // Time for writing a 2D debug layer
  Stage2Data                                 & stage2) // FIXME: adding to stage2 point cloud will have to be made thread safe
{
  // Prepping for threading layers here.  Since each layer is independent
  // in memory we 'should' be able to thread them with minimum locking.  This
  // will give us more CPU scaling.
  // FIXME: check reads/writes here thread safe and add any needed locking

  bool useDiffCache = true; // for now
  // ------------------------------------------------------------------------------
  // Do we try to output Stage 2 files for fusion2?
  // For the moment turn off stage2 if the --llg flag is on, since meteorologists
  // might be working on the first stage resolver.
  // const bool outputStage2 = !myWriteLLG;
  const bool outputStage2 = true;

  // FIXME: Feel like we could have a grid iterator to hide all this repeated stuff
  const LLCoverageArea& outg = myRadarGrid;
  AngleDegs startLat         = outg.getNWLat() - (outg.getLatSpacing() / 2.0); // move south (lat decreasing)
  AngleDegs startLon         = outg.getNWLon() + (outg.getLonSpacing() / 2.0); // move east (lon increasing)
  const auto heightsKM       = outg.getHeightsKM();

  // Set the value object for resolver
  VolumeValue vv;

  vv.cHeight        = myRadarCenter.getHeightKM(); // FIXME: Why not all location info?
  vv.layerHeightKMs = heightsKM[layer];

  auto& resolver = *myResolver;

  size_t totalLayer   = 0; // Total layer points
  size_t maskSkipped  = 0; // Skip by mask (roster nearest)
  size_t rangeSkipped = 0; // Skip by range (outside circle)
  size_t upperGood    = 0; // Have tilt above
  size_t lowerGood    = 0; // Have tilt below
  size_t sameTiltSkip = 0; // Enclosing tilts did not change
  size_t attemptCount = 0; // Resolver calculation

  // Each 2D layer is independent so threading here should be ok
  auto output    = myLLGCache->get(layer);
  auto& gridtest = output->getFloat2DRef();

  LogInfo("Processing " << vv.layerHeightKMs << " KM layer...\n");

  auto& llp = *myLLProjections[layer];
  auto& ssc = *(mySinCosCache);

  llp.reset();
  ssc.reset(); // FIXME: ssc is 2D thread conflict here, we will need random access (slightly slower)

  // Note: The 'range' is 0 to numY always, however the index to global grid is y+out.startY;
  AngleDegs atLat = startLat;

  for (size_t y = 0; y < outg.getNumY(); y++, atLat -= outg.getLatSpacing()) { // a north to south
    AngleDegs atLon = startLon;
    // Note: The 'range' is 0 to numX always, however the index to global grid is x+out.startX;
    for (size_t x = 0; x < outg.getNumX(); x++, atLon += outg.getLonSpacing()) { // a east to west row, changing lon per cell
      totalLayer++;

      // Warning to future me: These values stream so always get them or it will be off on next call
      // Cache get x, y of lat lon to the _virtual_ azimuth, elev, range of that cell center
      llp.get(vv.virtualAzDegs, vv.virtualElevDegs, vv.virtualRangeKMs);
      ssc.get(vv.sinGcdIR, vv.cosGcdIR);

      // Mask check..
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

      // Anything over terrain range Kms hard ignore.
      if (vv.virtualRangeKMs > myRangeKMs) {
        // Since this is outside range it should never be different from the initialization
        // gridtest[y][x] = Constants::DataUnavailable;
        rangeSkipped++;
        continue;
      }


      // Search the virtual volume for elevations above/below our virtual one
      // Linear for 'small' N of elevation volume is faster than binary here.  Interesting
      myElevationVolume->getSpreadL(vv.virtualElevDegs, levels, pointers, vv.getLower(),
        vv.getUpper(), vv.get2ndLower(),
        vv.get2ndUpper());
      if (vv.getLower() != nullptr) { lowerGood++; }
      if (vv.getUpper() != nullptr) { upperGood++; }

      // If the upper and lower for this point are the _same_ RadialSets as before, then
      // the interpolation will end up with the same data value.
      // This takes advantage that in a volume if a couple tilts change, then
      // the space time affected is fairly small compared to the entire volume's 3D coverage.
      if (useDiffCache) {
        const bool changedEnclosingTilts = llp.set(x, y, vv.getLower(), vv.getUpper(),
            vv.get2ndLower(), vv.get2ndUpper());
        if (!changedEnclosingTilts) { // The value won't change, so continue
          sameTiltSkip++;
          continue;
        }
      }

      attemptCount++;
      resolver.calc(vv);

      // Ok all resolvers pass 1 for the weight by default
      gridtest[y][x] = vv.dataValue / vv.dataWeight1;

      if (outputStage2) {
        stage2.add(vv.dataValue, vv.dataWeight1, x, y, layer);
      }
    }
  }

  // --------------------------------------------------------
  // Write debugging 2D LatLonGrid for layer (post entire layer processing)
  //
  if (myWriteLLG) {
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
  // FIXME: If we log this is a 'group' so we want to lock for multiple log
  // messages (otherwise these lines will interweave since we only lock at line level currently)
  // or we just print a single line (probably easier)
  const double percentAttempt = (double) (attemptCount) / (double) (totalLayer) * 100.0;

  LogInfo("    Mask, Range, Same enclosing tilts skipped: " << maskSkipped << ", " << rangeSkipped << ", " <<
    sameTiltSkip << "\n");
  LogInfo("    Resolved: " << attemptCount << " (" << percentAttempt << "%).\n");
  LogInfo("    Total points: " << totalLayer << "\n");
  LogInfo("----------------------------------------\n");

  return attemptCount;
} // RAPIOFusionOneAlg::processHeightLayer

void
RAPIOFusionOneAlg::processVolume(const Time rTime)
{
  // Don't do anything unless we got some new data since last time
  if (myDirty < 1) { return; }
  LogInfo("Processing full volume, " << myDirty << " file(s) have been received.\n");
  myDirty = 0;

  // Read the coverage mask (in real time it could change due to radars up/down)
  readCoverageMask();

  // Get the elevation volume pointers and levels for speed
  std::vector<double> levels;
  std::vector<DataType *> pointers;

  myElevationVolume->getTempPointerVector(levels, pointers);
  LogInfo(*myElevationVolume << "\n");

  // Keep stage 2 output code separate, cheap to make this if we don't use it
  LLCoverageArea& outg         = myRadarGrid;
  auto heightsKM               = outg.getHeightsKM();
  std::vector<size_t> bitsizes = { outg.getNumX(), outg.getNumY(), outg.getNumZ() };
  size_t totalLayer            = outg.getNumX() * outg.getNumY() * outg.getNumZ();
  Stage2Data stage2(myRadarName, myTypeName, myWriteOutputUnits, myRadarCenter, outg.getStartX(),
    outg.getStartY(),
    bitsizes);

  size_t attemptCount = 0;

  // Handle all layers
  for (size_t layer = 0; layer < heightsKM.size(); layer++) {
    attemptCount += processHeightLayer(layer, levels, pointers, rTime, stage2);
  }

  const double percentAttempt = (double) (attemptCount) / (double) (totalLayer) * 100.0;

  LogInfo("Total all layer grid points: " << totalLayer << " (" << percentAttempt << "%).\n");

  // Send stage2 data (note this is a full conus volume)
  bool outputStage2 = true;

  if (outputStage2) {
    LLH aLocation;
    // Time aTime  = Time::CurrentTime(); // Time depends on archive/realtime or incoming enough?
    Time aTime = rTime; // The time of the data matches the incoming data
    LogInfo("Sending stage2 data: " << myWriteStage2Name << " at " << aTime << "\n");
    stage2.send(this, aTime, myWriteStage2Name);
  }
} // RAPIOFusionOneAlg::processNewData

int
main(int argc, char * argv[])
{
  // Create and run fusion stage 1
  RAPIOFusionOneAlg alg = RAPIOFusionOneAlg();

  alg.executeFromArgs(argc, argv);
}
