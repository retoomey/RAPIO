#include "rFusion1.h"
#include "rBinaryTable.h"
#include "rConfigRadarInfo.h"
#include "rStage2Data.h"

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
  Volume::introduceSelf();
  // Volume::introduce("yourvolume", myVolumeClass); To add your own

  // -------------------------------------------------------------
  // VolumeValueResolver registration and creation
  VolumeValueResolver::introduceSelf();
  RobertLinear1Resolver::introduceSelf();
  LakResolver1::introduceSelf();
  // VolumeValueResolver::introduce("yourresolver", myResolverClass); To add your own

  // -------------------------------------------------------------
  // Terrain blockage registration and creation
  TerrainBlockage::introduceSelf();
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

  // -----------------------------------------------
  // Plugins

  // Volume value resolver plugin
  o.optional("resolver", "lak",
    "Value Resolver Algorithm, such as 'lak', or your own. Params follow: lak,params.");
  VolumeValueResolver::introduceSuboptions("resolver", o);

  // Elevation volume plugin
  o.optional("volume", "simple",
    "Volume algorithm, such as 'simple', or your own. Params follow: simple,params.");
  Volume::introduceSuboptions("volume", o);

  // Terrain blockage plugin
  o.optional("terrain", "lak",
    "Terrain blockage algorithm. Params follow: lak,/DEMS.  Most take root folder of DEMS.");
  TerrainBlockage::introduceSuboptions("terrain", o);
  o.addSuboption("terrain", "", "Don't apply any terrain algorithm.");
} // RAPIOFusionOneAlg::declareOptions

void
RAPIOFusionOneAlg::declareAdvancedHelp(RAPIOOptions& o)
{
  // Add advanced help.  This is only called if 'help' is asked for and also forces
  // loading of all dynamic modules and or modules from declarePlugins.
  // Basically this allows the plugins to show up under each option.
  o.addAdvancedHelp("resolver", VolumeValueResolver::introduceHelp());
  o.addAdvancedHelp("volume", Volume::introduceHelp());
  o.addAdvancedHelp("terrain", TerrainBlockage::introduceHelp());
}

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
  myResolverAlg      = o.getString("resolver");
  myTerrainAlg       = o.getString("terrain");
  myVolumeAlg        = o.getString("volume");
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

    /*
     * // For each of our height layers to process
     * for (size_t layer = 0; layer < g.getNumZ(); layer++) {
     * auto newone = myLLGCache->get(layer);
     *
     * newone->setReadFactory("netcdf"); // default to netcdf output if we write it
     * // Default the LatLonGrid to DataUnvailable, we'll fill in good values later
     * auto array = newone->getFloat2D();
     * array->fill(Constants::DataUnavailable);
     * }
     */
  }
}

void
RAPIOFusionOneAlg::createLLHtoAzRangeElevProjection(
  AngleDegs cLat, AngleDegs cLon, LengthKMs cHeight,
  LLCoverageArea& g)
{
  // We cache a bunch of repeated trig functions that save us a lot of CPU time
  if (myLLProjections[0] == nullptr) {
    LogInfo("-------------------------------Projection AzRangeElev cache generation.\n");
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
      // LogInfo("  Layer: " << myHeightsM[i] * 1000.0 << " meters.\n");
      LogInfo("  Layer: " << heightsKM[i] * 1000.0 << " meters.\n");
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
    LogInfo("-------------------------------Projection AzRangeElev cache complete.\n");
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

  // Radar center coordinates
  const LLH center        = r->getRadarLocation();
  const AngleDegs cLat    = center.getLatitudeDeg();
  const AngleDegs cLon    = center.getLongitudeDeg();
  const LengthKMs cHeight = center.getHeightKM();

  // Link to first incoming radar and moment, we will ignore any others from now on
  LogInfo(
    "Linking this algorithm to radar '" << radarName << "' and typename '" << typeName <<
      "' since first pass we only handle 1\n");
  myTypeName  = typeName;
  myRadarName = radarName;

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
  myResolver = VolumeValueResolver::createFromCommandLineOption(myResolverAlg);

  // Stubbornly refuse to run if Volume Value Resolver requested by name and not found or failed
  if (myResolver == nullptr) {
    LogSevere("Volume Value Resolver '" << myResolverAlg << "' requested, but failed to find and/or initialize.\n");
    exit(1);
  } else {
    LogInfo("Using Volume Value Resolver: '" << myResolverAlg << "'\n");
  }

  // -------------------------------------------------------------
  // Terrain blockage creation
  if (myTerrainAlg.empty()) {
    LogInfo("No terrain blockage algorithm requested.\n");
  } else {
    myTerrainBlockage = TerrainBlockage::createFromCommandLineOption(myTerrainAlg,
        r->getLocation(), myRangeKMs, radarName);
    // Stubbornly refuse to run if terrain requested by name and not found or failed
    if (myTerrainBlockage == nullptr) {
      LogSevere("Terrain blockage '" << myTerrainAlg << "' requested, but failed to find and/or initialize.\n");
      exit(1);
    } else {
      LogInfo("Using Terrain Blockage: '" << myTerrainBlockage << "'\n");
    }
  }

  // -------------------------------------------------------------
  // Elevation volume creation
  LogInfo(
    "Creating virtual volume for '" << myRadarName << "' and typename '" << myTypeName <<
      "'\n");
  myElevationVolume = Volume::createFromCommandLineOption(myVolumeAlg, myRadarName + "_" + myTypeName);
  // Stubbornly refuse to run if requested by name and not found or failed
  if (myElevationVolume == nullptr) {
    LogSevere("Volume '" << myVolumeAlg << "' requested, but failed to find and/or initialize.\n");
    exit(1);
  } else {
    LogInfo("Using Volume algorithm: '" << myVolumeAlg << "'\n");
  }

  // Look up from cells to az/range/elev for RADAR
  createLLHtoAzRangeElevProjection(cLat, cLon, cHeight, outg);

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
RAPIOFusionOneAlg::processNewData(rapio::RAPIOData& d)
{
  // Look for any data the system knows how to read...
  // auto r = d.datatype<rapio::DataType>();
  auto r = d.datatype<rapio::RadialSet>();

  if (r != nullptr) {
    // ------------------------------------------------------------------------------
    // Gather information on the current RadialSet
    //
    // FIXME: Actually I think with the shift to polar terrain we don't actually use azimuthSpacing.
    //        Polar only requires the azimuth center of the gate
    auto azimuthSpacingArray = r->getAzimuthSpacingVector();
    double azimuthSpacing    = 1.0;
    if (azimuthSpacingArray != nullptr) {
      auto& ref = azimuthSpacingArray->ref();
      azimuthSpacing = ref[0];
    }
    AngleDegs elevDegs    = r->getElevationDegs();
    const Time rTime      = r->getTime();
    const time_t dataTime = rTime.getSecondsSinceEpoch();
    // Radar center coordinates
    const LLH center        = r->getRadarLocation();
    const LengthKMs cHeight = center.getHeightKM();

    // Get the radar name and typename from this RadialSet.
    std::string name = "UNKNOWN";
    const std::string aTypeName = r->getTypeName();
    const std::string aUnits    = r->getUnits();
    if (!r->getString("radarName-value", name)) {
      LogSevere("No radar name found in RadialSet, ignoring data\n");
      return;
    }

    LogInfo(
      " " << r->getTypeName() << " (" << r->getNumRadials() << " * " << r->getNumGates() << ")  Radials * Gates,  Time: " << r->getTime() <<
        "\n");

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
      LogInfo("--->Applying Lak's moving average smoothing filter to radialset\n");
      LogInfo("    Filter ratio scale is " << scale_factor << "\n");
      LakRadialSmoother::smooth(r, scale_factor / 2);
    } else {
      LogInfo("--->Not applying smoothing since scale factor is " << scale_factor << "\n");
    }

    // Assign the ID key for cache storage.  Note the size matters iff you have more
    // DataTypes to keep track of than the key size.  Currently FusionKey defines the key
    // size and max unique elevations we can hold at once.
    static FusionKey keycounter = 0; // 0 is reserved for nothing and not used
    if (++keycounter == 0) { keycounter = 1; } // skip 0 again on overflow
    r->setID(keycounter);
    LogInfo("RadialSet short key: " << (int) keycounter << "\n");

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
      myTerrainBlockage->calculateTerrainPerGate(r);
    }

    // ------------------------------------------------------------------------------
    // Do we try to output Stage 2 files for fusion2?
    // For the moment turn off stage2 if the --llg flag is on, since meteorologists
    // might be working on the first stage resolver.
    const bool outputStage2 = !myWriteLLG;

    // Set the value object for resolvers
    VolumeValue vv;
    vv.cHeight = cHeight;

    // Get the elevation volume pointers and levels for speed
    std::vector<double> levels;
    std::vector<DataType *> pointers;
    myElevationVolume->getTempPointerVector(levels, pointers);
    LogInfo(*myElevationVolume << "\n");
    // F(Lat,Lat,Height) --> Virtual Az, Elev, Range projection add spacing/2 to get cell cellcenters
    AngleDegs startLat = myRadarGrid.getNWLat() - (myRadarGrid.getLatSpacing() / 2.0); // move south (lat decreasing)
    AngleDegs startLon = myRadarGrid.getNWLon() + (myRadarGrid.getLonSpacing() / 2.0); // move east (lon increasing)

    // Each layer of merger we have to loop through
    LLCoverageArea outg = myRadarGrid;

    // Keep stage 2 output code separate, cheap to make this if we don't use it
    std::vector<size_t> bitsizes = { outg.getNumX(), outg.getNumY(), outg.getNumZ() };
    Stage2Data stage2(myRadarName, myTypeName, myWriteOutputUnits, center, outg.getStartX(), outg.getStartY(),
      bitsizes);

    auto& resolver    = *myResolver;
    bool useDiffCache = false;
    size_t total      = 0;
    auto heightsKM    = outg.getHeightsKM();
    // for (size_t layer = 0; layer < myHeightsM.size(); layer++) {
    for (size_t layer = 0; layer < heightsKM.size(); layer++) {
      // vv.layerHeightKMs = myHeightsM[layer] / 1000.0;
      // vv.layerHeightKMs = myHeightsM[layer]; // / 1000.0;
      vv.layerHeightKMs = heightsKM[layer]; // / 1000.0;

      // FIXME: Do we put these into the output?
      size_t totalLayer   = 0;
      size_t rangeSkipped = 0; // Skip by range (outside circle)
      size_t upperGood    = 0; // Have tilt above
      size_t lowerGood    = 0; // Have tilt below
      size_t sameTiltSkip = 0;

      size_t attemptCount    = 0;
      size_t differenceCount = 0;

      auto output = myLLGCache->get(layer);
      // Update output time in case we write it out for debugging
      output->setTime(r->getTime());

      auto& gridtest = output->getFloat2DRef();

      LogInfo("Starting processing loop for height " << vv.layerHeightKMs * 1000 << " meters...\n");
      ProcessTimer process1("Actual calculation guess of speed: ");

      auto& llp = *myLLProjections[layer];
      auto& ssc = *(mySinCosCache);
      llp.reset();
      ssc.reset();

      // Note: The 'range' is 0 to numY always, however the index to global grid is y+out.startY;
      AngleDegs atLat = startLat;
      for (size_t y = 0; y < outg.getNumY(); y++, atLat -= outg.getLatSpacing()) { // a north to south
        AngleDegs atLon = startLon;
        // Note: The 'range' is 0 to numX always, however the index to global grid is x+out.startX;
        for (size_t x = 0; x < outg.getNumX(); x++, atLon += outg.getLonSpacing()) { // a east to west row, changing lon per cell
          total++;
          totalLayer++;
          // Cache get x, y of lat lon to the _virtual_ azimuth, elev, range of that cell center
          llp.get(azimuthSpacing, vv.virtualAzDegs, vv.virtualElevDegs, vv.virtualRangeKMs);
          ssc.get(vv.sinGcdIR, vv.cosGcdIR);

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

          // I'm gonna start with the pointers as keys, which are numbers, but it's 'possible'
          // a new radialset could get same pointer (expire/new).
          // FIXME: Probably super rare to get same pointer, but maybe we add
          // a RadialSet or general DataType counter at some point to be sure.
          // Humm.  The tilt time 'might' work appending it to the pointer value.

          // Think this is failing actually...it isn't always set depends on resolver
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

          // Actually write to the value cache
          // FIXME: Difference test may require multivalue probably including weights...
          //        Time might be an issue here..this will increase gridtest size/cache
          if (gridtest[y][x] != vv.dataValue) {
            differenceCount++;
            gridtest[y][x] = vv.dataValue;
            if (outputStage2) {
              stage2.add(vv.dataValue, vv.dataWeight1, x, y, layer);
            }
          }
        }
      }

      // --------------------------------------------------------
      // Write LatLonGrid or Raw merger files (FIXME: Maybe more flag control)
      //

      static int writeCount = 0;
      if (++writeCount >= myThrottleCount) {
        if (myWriteLLG) {
          writeOutputCAPPI(output);
        } else {
          // Write any per tilt stage2 files.  I think we're just writing per
          // 33 layer files, but never know
        }
        writeCount = 0;
      }

      // --------------------------------------------------------
      // Log info on the layer calculations
      //
      double percentAttempt = (double) (attemptCount) / (double) (totalLayer) * 100.0;
      LogInfo("  Completed Layer " << vv.layerHeightKMs << " KMs. " << totalLayer << " left.\n");
      totalLayer -= rangeSkipped;
      LogInfo("    Range skipped: " << rangeSkipped << ". " << totalLayer << " left.\n");
      totalLayer -= sameTiltSkip;
      LogInfo("    Same cached upper/lower skip: " << sameTiltSkip << ". " << totalLayer << " left.\n");
      LogInfo("    Attempting: " << attemptCount << " or " << percentAttempt << " %.\n");
      LogInfo("      Difference count (number sent to stage2): " << differenceCount << "\n");
      LogInfo("      _New_ upper/Lower good hits: " << upperGood << " and " << lowerGood << "\n");
      LogInfo("----------------------------------------\n");
      // break;  Test for quick writing Stage2 files
    } // END HEIGHT LOOP

    LogInfo("Total all layer grid points: " << total << "\n");

    // Send stage2 data (note this is a full conus volume)
    if (outputStage2) {
      LLH aLocation;
      // Time aTime  = Time::CurrentTime(); // Time depends on archive/realtime or incoming enough?
      Time aTime = rTime; // The time of the data matches the incoming data
      LogSevere("Sending " << myWriteStage2Name << " at " << aTime << "\n");
      stage2.send(this, aTime, myWriteStage2Name);
    }

    // ----------------------------------------------------------------------------
  }
} // RAPIOFusionOneAlg::processNewData

int
main(int argc, char * argv[])
{
  // Create and run fusion stage 1
  RAPIOFusionOneAlg alg = RAPIOFusionOneAlg();

  alg.executeFromArgs(argc, argv);
}
