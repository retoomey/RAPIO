#include "rFusion1.h"
#include "rBinaryTable.h"
#include "rVolumeValueResolver.h"
#include "rConfigRadarInfo.h"

using namespace rapio;

/** Showing elevation angle of the contributing tilt below us */
class TestResolver1 : public VolumeValueResolver
{
public:
  virtual void
  calc(VolumeValue& vv) override
  {
    if (vv.lower != nullptr) {
      RadialSet& lowerr = *((RadialSet *) vv.lower);
      auto angle        = lowerr.getElevationDegs();
      vv.dataValue = angle;
    } else {
      vv.dataValue = Constants::MissingData;
    }
  }
};

class RobertLinear1Resolver : public VolumeValueResolver
{
public:
  virtual void
  calc(VolumeValue& vv) override
  {
    // ------------------------------------------------------------------------------
    // Query information for above and below the location
    bool haveLower = queryLayer(vv, vv.lower, vv.lLayer);
    bool haveUpper = queryLayer(vv, vv.upper, vv.uLayer);

    // ------------------------------------------------------------------------------
    // Filter out values where terrain blockage more than 50%
    // FIXME: Part of me wonders if terrain height should be here to, but we'd need a field
    // for it.
    if (vv.lLayer.terrainPercent > .50) {
      vv.lLayer.value = Constants::DataUnavailable;
      haveLower       = false;
    }
    if (vv.lLayer.terrainPercent > .50) {
      vv.lLayer.value = Constants::DataUnavailable;
      haveUpper       = false;
    }

    // ------------------------------------------------------------------------------
    // In beamwidth calculation
    //
    // FIXME: might be able to optimize by not always calculating in beam width,
    // but it's making my head hurt so just do it always for now
    // The issue is between tilts lots of tan, etc. we might skip

    // Get height of a .5 deg higher (half beamwidth of 1)
    LengthKMs lowerHeightKMs;
    bool inLowerBeamwidth = false;

    if (haveLower) {
      heightForDegreeShift(vv, vv.lower, .5, lowerHeightKMs);
      inLowerBeamwidth = (vv.layerHeightKMs <= lowerHeightKMs);
    }

    // Get height of a .5 deg lower (half beamwidth of 1)
    LengthKMs upperHeightKMs;
    bool inUpperBeamwidth = false;

    if (haveUpper) {
      heightForDegreeShift(vv, vv.upper, -.5, upperHeightKMs);
      inUpperBeamwidth = (vv.layerHeightKMs >= upperHeightKMs);
    }

    // ------------------------------------------------------------------------------
    // Background MASK calculation
    //
    // Keep background mask logic separate for now, though would probably be faster
    // doing it in the value calculation.  I recommend doing it this way because
    // logically it frys your brain a bit less.  You can 'have' and upper tilt,
    // but have bad values or missing, etc..so the cases are not one to one
    double v = Constants::DataUnavailable;

    if (haveUpper && haveLower) { // 11 Four binary possibilities
      v = Constants::MissingData;
    } else {
      if (haveUpper) { // 10
        if (inUpperBeamwidth) {
          v = Constants::MissingData;
        }
      } else if (haveLower) { // 01
        if (inLowerBeamwidth) {
          v = Constants::MissingData;
        }
      } else {
        // v = Constants::DataUnavailable; // 00
      }
    }

    const double& lValue = vv.lLayer.value;
    const double& uValue = vv.uLayer.value;

    // FIXME: Apply terrain to good data values, right?
    // This depends on if we want to 'see' stage one, or passing terrain and values to stage two
    // for moment I'll apply it
    if (Constants::isGood(lValue) && Constants::isGood(uValue)) {
      // Linear interpolate using heights.  With two values we can do interpolation
      // between the values always, either linear or exponential
      double wt = (vv.layerHeightKMs - vv.lLayer.heightKMs) / (upperHeightKMs - vv.uLayer.heightKMs);
      if (wt < 0) { wt = 0; } else if (wt > 1) { wt = 1; }
      const double nwt = (1.0 - wt);

      const double lTerrain = lValue * (1 - vv.lLayer.terrainPercent);
      const double uTerrain = uValue * (1 - vv.uLayer.terrainPercent);

      // v = (0.5 + nwt * lValue + wt * uValue);
      v = (0.5 + nwt * lTerrain + wt * uTerrain);
    } else if (inLowerBeamwidth) {
      if (Constants::isGood(lValue)) {
        const double lTerrain = lValue * (1 - vv.lLayer.terrainPercent);
        v = lTerrain;
      } else {
        v = lValue; // Use the gate value even if missing, RF, unavailable, etc.
      }
    } else if (inUpperBeamwidth) {
      if (Constants::isGood(uValue)) {
        const double uTerrain = uValue * (1 - vv.uLayer.terrainPercent);
        v = uTerrain;
      } else {
        v = uValue;
      }
    }
    #if 0
  } else if (Constants::isGood(vv.lLayer.value)) {
    if (inLowerBeamwidth) {
      v = lValue;
    }
  } else if (Constants::isGood(vv.vUpper)) {
    if (inUpperBeamwidth) {
      v = uValue;
    }
  }
    #endif

    vv.dataValue = v;
  } // calc
};

// End Resolvers
// -----------------------------------------------------------------------------------------------------

/*
 * New merger stage 1, focusing on specialized on-the-fly caching
 *
 * @author Robert Toomey
 **/
void
RAPIOFusionOneAlg::declareOptions(RAPIOOptions& o)
{
  o.setDescription(
    "RAPIO Fusion Stage One Algorithm.  Designed to run for a single radar/moment, this creates .raw files for the MRMS stage 2 merger.");
  o.setAuthors("Robert Toomey");

  // legacy use the t, b and s to define a grid
  o.declareLegacyGrid();

  o.boolean("llg", "Turn on/off writing output LatLonGrids per level");

  // Terrain DEM location folder with files of form RADAR.nc
  // Algorithm will stop if terrain cannot be found here for radar (for non-blank)
  o.optional("terrain", "", "Path to terrain files of form RADAR.nc, otherwise we ignore terrain clipping.");

  // Range to use in KMs.  Default is 460.  This determines subgrid and max range of valid
  // data for the radar
  o.optional("rangekm", "460", "Range in kilometers for radar.");
}

/** RAPIOAlgorithms process options on start up */
void
RAPIOFusionOneAlg::processOptions(RAPIOOptions& o)
{
  // FIXME: Feel like algorithms need a general init not just the process options
  // Maybe we just rename this method to be clearer...
  // LogInfo("Checking grid variables: "<<o.getString("t")<<", " << o.getString("b") << ", " << o.getString("s") << "\n");

  // -----------------------------------------------------------------------------------
  // Lak does the gridspecification and heightspacing which does a ton of stuff.
  // I think the GridSpecification actually slows the loop massively, since it
  // abstractly redoes the index math for each cell.  The heights are
  // a list of output heights.  For the moment, I'm gonna hardcode the NMQWD
  // heights in a vector for testing since well we might not be able to actually
  // 'do' merger using this new way anyway
  // FIXME: Do all the configuration stuff
  //
  // And we don't have a 20..what strange logic
  // -t "55 -130 20" -b "20 -60 0.5" -s "0.01 0.01 NMQWD"
  //
  // <heightlevels name="NMQWD">
  // <level incr="250" upto="3000"/>
  // <level incr="500" upto="9000"/>
  // <level incr="1000" upto="infty"/>
  // </heightlevels>
  ///core/configs/merger_configs/HAWAII_configs.xml
  //
  //
  #if 1
  myHeightsM = {
    // up to 3000 starting from 1 km
    500,     750,  1000,  1250,  1500,  1750,  2000,  2250,  2500, 2750, 3000,       // 0-10
    3500,   4000,  4500,  5000,  5500,  6000,  6500,  7000,  7500, 8000, 8500, 9000, // 11-22
    10000, 11000, 12000, 13000, 14000, 15000, 16000, 17000, 18000, 19000             // 23-32
  };
  for (size_t hh = 0; hh < myHeightsM.size(); hh++) {
    myHeightsIndex.push_back(hh);
  }
  #else
  myHeightsM = {
    // up to 3000 starting from 1 km
    // 500, 2000, 9000
    // 500, 2000, 5000
    5000
  };
  // We need the Z component for each height for outputting the layers, this
  // hack is only needed because our current stage 2 has 33 layers and we're sparse doing them
  myHeightsIndex = {
    // 0, 6, 14
    14
  };
  #endif // if 0
  myLLProjections.resize(myHeightsM.size());
  if (myLLProjections.size() < 1) {
    LogSevere("No height layers to process, exiting\n");
    exit(1);
  }

  LogInfo("We're hard locked to the following NMQWD output heights:\n")
  for (auto x:myHeightsM) {
    std::cout << x / 1000.0 << " ";
  }
  std::cout << "\n";

  o.getLegacyGrid(myFullGrid);

  myWriteLLG = o.getBoolean("llg");
  myTerrainPath = o.getString("terrain");
  myRangeKMs = o.getFloat("rangekm");
  if (myRangeKMs < 50) {
    myRangeKMs = 50;
  } else if (myRangeKMs > 1000) {
    myRangeKMs = 1000;
  }
  LogInfo("Range range is " << myRangeKMs << " Kilometers\n");
} // RAPIOFusionOneAlg::processOptions

void
RAPIOFusionOneAlg::createLLGCache(std::shared_ptr<RadialSet> r, const LLCoverageArea& g,
  const std::vector<double>& heightsM)
{
  if (myLLGCache.size() == 0) {
    ProcessTimer("Creating initial LLG value cache.");
    const std::string aName = r->getTypeName() + "Fused";
    std::string aUnits = r->getUnits();
    if (aUnits.empty()) {
      LogSevere("Units still wonky because of W2 bug, forcing dBZ at moment..\n");
      aUnits = "dBZ";
    }


    for (size_t layer = 0; layer < heightsM.size(); layer++) {
      const LengthKMs layerHeightKMs = heightsM[layer] / 1000.0;

      std::shared_ptr<LatLonGrid> output = LatLonGrid::
        Create(aName, aUnits, LLH(g.nwLat, g.nwLon, layerHeightKMs), Time(), g.latSpacing, g.lonSpacing,
          g.numY, g.numX);
      output->setReadFactory("netcdf"); // default to netcdf output if we write it
      auto array = output->getFloat2D();
      array->fill(Constants::DataUnavailable);

      // Ref to the actual raw array for speed
      // auto& data = array->ref();

      myLLGCache.add(output);
    }
  }
}

void
RAPIOFusionOneAlg::createLLHtoAzRangeElevProjection(
  AngleDegs cLat, AngleDegs cLon, LengthKMs cHeight,
  std::shared_ptr<TerrainBlockageBase> terrain,
  LLCoverageArea& g)
{
  /** Start by assuming azimuth spacing of 1 degree. */
  double terrainSpacing = 1.0;

  /** Projection cache creation, this is still slow.  However, we 'could' auto cache this maybe to disk */
  if (myLLProjections[0] == nullptr) {
    LogInfo("-------------------------------Projection/Terrain cache generation.\n");
    LengthKMs rangeKMs;
    AngleDegs azDegs, virtualElevDegs;

    // Create the sin/cos cache.  We just need one of these vs the Az cache below
    // that is per conus layer
    mySinCosCache = std::make_shared<SinCosLatLonCache>(g.numX, g.numY);

    // LogInfo("Projection LatLonHeight to AzRangeElev cache creation...\n");
    for (size_t i = 0; i < myLLProjections.size(); i++) {
      const LengthKMs layerHeightKMs = myHeightsM[i] / 1000.0;
      LogInfo("  Layer: " << myHeightsM[i] << " meters.\n");
      myLLProjections[i] = std::make_shared<AzRanElevTerrainCache>(g.numX, g.numY);
      auto& llp = *(myLLProjections[i]);
      auto& ssc = *(mySinCosCache);

      llp.reset();
      ssc.reset();
      // Would we save speed by caching all the lat/lon values?
      AngleDegs startLat = g.nwLat - (g.latSpacing / 2.0); // move south (lat decreasing)
      AngleDegs startLon = g.nwLon + (g.lonSpacing / 2.0); // move east (lon increasing)
      AngleDegs atLat = startLat;

      // We'll skip terrain once we're so high we're not hitting in a layer
      // This assumes the layers are ascending
      bool tooHighForTerrain = false;

      size_t terrainHitCount = 0;                                  // Count hits
      for (size_t y = 0; y < g.numY; y++, atLat -= g.latSpacing) { // a north to south
        AngleDegs atLon = startLon;
        for (size_t x = 0; x < g.numX; x++, atLon += g.lonSpacing) { // a east to west row, changing lon per cell
          //
          // The sin/cos attenuation factors from radar center to lat lon
          // This doesn't need height so it can be just a single layer
          double sinGcdIR, cosGcdIR;
          if (i == 0) {
            Project::stationLatLonToTarget(
              atLat, atLon, cLat, cLon, sinGcdIR, cosGcdIR);
            ssc.add(sinGcdIR, cosGcdIR); // move forward
          } else {
            ssc.get(sinGcdIR, cosGcdIR); // move forward
          }

          // This is still relatively slow 2 secs or so.  FIXME: Clip to radar range
          // There's more here to be improved...caching sin cos of station center for instance
          // Project::BeamPath_LLHtoAzRangeElev(atLat, atLon, layerHeightKMs,
          //  cLat, cLon, cHeight, virtualElevDegs, azDegs, rangeKMs);
          Project::Cached_BeamPath_LLHtoAzRangeElev(atLat, atLon, layerHeightKMs,
            cLat, cLon, cHeight, sinGcdIR, cosGcdIR, virtualElevDegs, azDegs, rangeKMs);
          // FIXME: compare original to the cache value make sure didn't break.
          //
          // auto percent2 = terrain->computePercentBlocked(
          //  1, // Technically not static, but start with 1 deg
          //  .5,
          //  240, .3);
          // LogSevere("OUTPUT NEW: " << (int) percent2 << "\n");
          // exit(1);

          #if 0
          unsigned char percent;
          if (myDEM != nullptr) {
            // Terrain percentage cache, value per 3D cell
            // Ok we have an issue here.  I can do terrain in 3D space, but the 'true' values of the
            // tilts are what are actually blocked or not. In other words, terrain has to be applied
            // to each of the upper/lower tilts and used to toss out data, not applied in 3D space on final values.
            percent = terrain->computePercentBlocked(
              terrainSpacing, // Technically not static, but start with 1 deg
              virtualElevDegs,
              azDegs, rangeKMs);
          } else {
            percent = 0; // 0 percent blocked
          }
          if (percent > 5) { terrainHitCount++; }
          #endif // if 0
          unsigned char percent = 0;

          llp.add(azDegs, virtualElevDegs, rangeKMs, percent);
        }
      }

      // Ok so if NO greater than 50 percent hits...we could turn off calculation of terrain
      // from this point on...
      if (terrainHitCount == 0) { }
      LogSevere("LAYER TERRAIN HITS: " << layerHeightKMs << " KMs :" << terrainHitCount << "\n");
    }
    LogInfo("-------------------------------Projection/Terrain cache complete.\n");
    // LogSevere("FORCED EXITING TEST\n");
    // exit(1);
  }
} // RAPIOFusionOneAlg::createLLHtoAzRangeElevProjection

void
RAPIOFusionOneAlg::processNewData(rapio::RAPIOData& d)
{
  const LengthKMs TerrainRange = myRangeKMs;

  // Look for any data the system knows how to read...
  // auto r = d.datatype<rapio::DataType>();
  auto r = d.datatype<rapio::RadialSet>();

  if (r != nullptr) {
    // Lak used azimuth spacing of first radial.  Could we use the spacing of each radial?
    // It probably "doesn't" change would be my guess
    // We need this for the terrain algorithm
    auto azimuthSpacingArray = r->getAzimuthSpacingVector();
    double azimuthSpacing = 1.0;
    if (azimuthSpacingArray != nullptr) {
      auto& ref = azimuthSpacingArray->ref();
      azimuthSpacing = ref[0];
    }
    AngleDegs elevDegs = r->getElevationDegs();
    const Time rTime = r->getTime();
    const time_t dataTime = rTime.getSecondsSinceEpoch();
    RObsBinaryTable::mrmstime aMRMSTime;
    aMRMSTime.epoch_sec = rTime.getSecondsSinceEpoch();
    aMRMSTime.frac_sec = rTime.getFractional();

    // ProcessTimer out("Startup for a single tile took\n");

    LogInfo(
      " " << r->getTypeName() << " (" << r->getNumRadials() << " * " << r->getNumGates() << ")  Radials * Gates,  Time: " << r->getTime() <<
        "\n");

    // ----------------------------------------------------------------------------
    // For the moment, we can only be linked to one radar and one typename
    // such as reflectivity

    // Get the radar name and typename from this RadialSet.
    std::string name = "UNKNOWN";
    const std::string aTypeName = r->getTypeName();
    const std::string aUnits = r->getUnits();
    if (r->getString("radarName-value", name)) {
      // LogInfo("      Radar name is found!  It is  '" << name << "'\n");
      if (myRadarName.empty()) {
        LogInfo(
          "Linking this algorithm to radar '" << name << "' and typename '" << aTypeName <<
            "' since first pass we only handle 1\n");
        myTypeName = aTypeName;
        myRadarName = name;
      } else {
        if ((myRadarName != name) || (myTypeName != aTypeName)) {
          LogSevere(
            "We are linked to '" << myRadarName << "-" << myTypeName << "', ignoring radar/typename '" << name << "-" << aTypeName <<
              "'\n");
          return;
        }
      }
    } else {
      LogSevere("No radar name found in RadialSet, ignoring data\n");
      return;
    };

    // ----------------------------------------------------------------------------
    // Every RADAR requires a Terrain object

    // Attempt to read a terrain DEM file...
    // We could lazy read based on incoming radar, 'maybe'.  Is radar name in the
    // radial set lol
    if (!myTerrainPath.empty()) {
      if (myDEM == nullptr) {
        LogInfo("-------------------------------DEM creation start\n");
        std::string file = myTerrainPath + "/" + name + ".nc";
        auto d = IODataType::read<LatLonGrid>(file);
        if (d != nullptr) {
          LogInfo("Terrain DEM read: " << file << "\n");
          myDEM = d;
        } else {
          LogSevere("Failed to read in Terrain DEM LatLonGrid at " << file << "\n");
          exit(1);
        }
        LogInfo("-------------------------------DEM creation start\n");
      }
    } else {
      LogInfo("No terrain path specified, so no terrain blockage will occur.\n");
    }

    // ----------------------------------------------------------------------------
    // Every RADAR/MOMENT will require a elevation volume for virtual volume


    // Create an elevation volume off the radial set, this stores a time clippable
    // virtual volume
    if (myElevationVolume == nullptr) {
      LogInfo(
        "Creating virtual volume and terrain blockage for '" << myRadarName << "' and typename '" << myTypeName <<
          "'\n");
      myElevationVolume = std::make_shared<ElevationVolume>(myRadarName + "_" + myTypeName);

      // Terrain blockage algorithm is a lookup with a given range and density
      myTerrainBlockage = nullptr;
      if (myDEM != nullptr) {
        // FIXME: Flag at some point
        // myTerrainBlockage =
        //  std::make_shared<TerrainBlockage>(myDEM, r->getLocation(), TerrainRange, name);
        myTerrainBlockage =
          std::make_shared<TerrainBlockage2>(myDEM, r->getLocation(), TerrainRange, name);
      }
    }

    // Always add to elevation volume
    myElevationVolume->addDataType(r);

    // ----------------------------------------------------------------------------
    // Every Unique RadialSet product will require a RadialSetLookup.
    r->getProjection(); // Creates RadialSetLookup..the resolution is less than Terrain,
                        // which is interesting.  FIXME: Make resolutions match?

    // Every RadialSet will require terrain per gate for filters
    if (myTerrainBlockage != nullptr) {
      myTerrainBlockage->calculateTerrainPerGate(r);
    }

    // Radar center coordinates
    const LLH center = r->getRadarLocation();
    const AngleDegs cLat = center.getLatitudeDeg();
    const AngleDegs cLon = center.getLongitudeDeg();
    const LengthKMs cHeight = center.getHeightKM();

    // Could cache this stuff too
    static bool firstTime = true;
    if (firstTime) {
      const LengthKMs fudgeKMs = 5; // Extra to include so circle is inside box a bit
      myRadarGrid = myFullGrid.insetRadarRange(cLat, cLon, TerrainRange + fudgeKMs);
      LogInfo("Radar center:" << cLat << "," << cLon << " at " << cHeight << " KMs\n");
      LogInfo("Full coverage grid: " << myFullGrid << "\n");
      firstTime = false;
    }
    LLCoverageArea outg = myRadarGrid;
    LogInfo("Radar subgrid: " << outg << "\n");

    // Get the elevation volume pointers and levels for speed
    std::vector<double> levels;
    std::vector<DataType *> pointers;
    myElevationVolume->getTempPointerVector(levels, pointers);
    LogInfo(*myElevationVolume << "\n");

    // F(Lat,Lat,Height) --> Virtual Az, Elev, Range projection add spacing/2 to get cell cellcenters
    AngleDegs startLat = outg.nwLat - (outg.latSpacing / 2.0); // move south (lat decreasing)
    AngleDegs startLon = outg.nwLon + (outg.lonSpacing / 2.0); // move east (lon increasing)
    createLLHtoAzRangeElevProjection(cLat, cLon, cHeight, myTerrainBlockage, outg);

    // We actually store a Lat Lon Grid per each CAPPI
    createLLGCache(r, outg, myHeightsM);

    // ------------------------------------------------------------------------------
    // RAW file entry storage
    //
    // Entries to write out eventually
    std::shared_ptr<RObsBinaryTable> entries = std::make_shared<RObsBinaryTable>();
    entries->setReadFactory("raw"); // default to raw output if we write it
    auto& t = *entries;

    // FIXME: refactor/methods, etc. This isn't clean by a long shot
    t.radarName = myRadarName;
    t.vcp = -9999; // Ok will merger be ok with this, need to check
    t.elev = elevDegs;
    t.typeName = r->getTypeName();
    t.setUnits(aUnits);
    t.markedLinesCacheFile = ""; // We ignore marked lines.  We'll see how it goes...
    t.lat = cLat;                // lat degrees
    t.lon = cLon;                // lon degrees
    t.ht = cHeight;              // kilometers assumed by w2merger
    t.data_time = dataTime;
    // Ok Lak guesses valid time based on vcp. What do we do here?
    // I'm gonna use the provided history value
    // If (valid_time < 1 || valid_time < blendingInterval ){
    //   valid_time = blendingInterval;
    // }
    t.valid_time = getMaximumHistory().seconds(); // Use seconds of our -h option?
    // End RAW file entry storage
    // ------------------------------------------------------------------------------

    // Set the value object for resolvers
    VolumeValue vv;
    vv.cHeight = cHeight;

    // Resolver
    RobertLinear1Resolver resolver;
    // TestResolver1 resolver;

    LengthKMs rangeKMs;

    // Each layer of merger we have to loop through
    size_t total = 0;
    for (size_t layer = 0; layer < myHeightsM.size(); layer++) {
      vv.layerHeightKMs = myHeightsM[layer] / 1000.0;

      // FIXME: Do we put these into the output?
      size_t totalLayer = 0;
      size_t rangeSkipped = 0;   // Skip by range (outside circle)
      size_t terrainBlocked = 0; // Skip by terrain > 50 blocked
      size_t upperGood = 0;      // Have tilt above
      size_t lowerGood = 0;      // Have tilt below
      size_t sameTiltSkip = 0;

      size_t attemptCount = 0;
      size_t differenceCount = 0;

      auto output = myLLGCache.get(layer);
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
      for (size_t y = 0; y < outg.numY; y++, atLat -= outg.latSpacing) { // a north to south
        AngleDegs atLon = startLon;
        // Note: The 'range' is 0 to numX always, however the index to global grid is x+out.startX;
        for (size_t x = 0; x < outg.numX; x++, atLon += outg.lonSpacing) { // a east to west row, changing lon per cell
          total++;
          totalLayer++;
          // Cache get x, y of lat lon to the _virtual_ azimuth, elev, range of that cell center
          unsigned char terrainPercent;
          llp.get(azimuthSpacing, vv.azDegs, vv.virtualElevDegs, rangeKMs, terrainPercent);
          ssc.get(vv.sinGcdIR, vv.cosGcdIR);

          // Create lat lon grid of a particular field...
          // gridtest[y][x] = rangeKMs; continue; // Range circles
          // gridtest[y][x] = vv.azDegs; continue;   // Azimuth

          // Anything over terrain range Kms hard ignore.
          if (rangeKMs > TerrainRange) {
            // Since this is outside range it should never be different from the initialization
            // gridtest[y][x] = Constants::DataUnavailable;
            rangeSkipped++;
            continue;
          }

          // Terrain filter, old merger skipped blockage over 50%
          // Wow, it's possible that all the terrain work is just being done, JUST to
          // test over 50% so we drop the point.  That seems sooo silly.
          /// SOB can't skip...need the values to be filtered by terrain percentage per radialset
          //          if (terrainPercent >= 50) { // Let me know a terrain hit for moment
          //            terrainBlocked++;
          // gridtest[y][x] = 20; // Do a range check...
          //            continue;
          //          }

          // Search the virtual volume for elevations above/below our virtual one
          // Linear for 'small' N of elevation volume is faster than binary here.  Interesting
          myElevationVolume->getSpreadL(vv.virtualElevDegs, levels, pointers, vv.lower, vv.upper);
          if (vv.lower != nullptr) { lowerGood++; }
          if (vv.upper != nullptr) { upperGood++; }

          // If the upper and lower for this point are the _same_ RadialSets as before, then
          // the interpolation will end up with the same data value.  This takes advantage that in a volume if say one tilt
          // expires and one tilt comes in, the space time affected is fairly small compared to the entire
          // volume.
          // I'm gonna start with the pointers as markers, which are numbers, but it's 'possible'
          // a new radialset could get same pointer, so we will add
          // a RadialSet or general DataType counter or something at some point.
          //
          // Maybe: In theory you could save more time by checking if just one changed and calculating one while
          // pulling the other heights/values from a cache.  I'd have to cache those heights then.
          const bool changedEnclosingTilts = llp.set(x, y, vv.lower, vv.upper);
          if (!changedEnclosingTilts) { // The value won't change, so continue
            sameTiltSkip++;
            continue;
          }

          attemptCount++;

          resolver.calc(vv);

          // Actually write to the value cache
          if (gridtest[y][x] != vv.dataValue) {
            differenceCount++;
            gridtest[y][x] = vv.dataValue;

            // Add entries

            // We already scaled...so what to push here.  It might be double applied?
            // no wait this would be weight for combining multiple radar values
            //   t.elevWeightScaled.push_back(int (0.5+(1+90)*100) );

            #define ELEV_SCALE(x) ( int(0.5 + (x + 90) * 100) )
            #define RANGE_SCALE 10

            // I think Lak rotates or flips
            const size_t outX = x + outg.startX;
            if (outX > myFullGrid.numX) {
              LogSevere(" CRITICAL X " << outX << " > " << myFullGrid.numX << "\n");
              exit(1);
            }
            const size_t outY = y + outg.startY;
            if (outY > myFullGrid.numY) {
              LogSevere(" CRITICAL Y " << outY << " > " << myFullGrid.numY << "\n");
              exit(1);
            }
            // Just realized Z will be wrong, since the stage 2 will assume all 33 layers, and
            // we're just doing subsets maybe.  So we'll need to figure that out
            // Lak: 33 2200 2600 as X, Y, Z.  Which seems non-intuitive to me.
            // Me: 2200 2600 33 as X, Y, Z.
            const size_t atZ = myHeightsIndex[layer]; // hack sparse height layer index (or we make object?)
            t.addRawObservation(atZ, outX, outY, vv.dataValue,
              int(rangeKMs * RANGE_SCALE + 0.5),
              //	  entry.elevWeightScaled(), Can't figure this out.  Why a individual scale...oh weight because the
              //	  cloud is the full cube.  Ok so we take current elev..no wait the virtual elevation and scale to char...
              //	  takes -90 to 90 to 0-18000, can still fit into unsigned char...
              //
              ELEV_SCALE(vv.virtualElevDegs),
              vv.azDegs, aMRMSTime);

            // Per point (I'm gonna have to rework it if I do a new stage 2):
            // t.azimuth  vector of azimuth of the point, right?  Ok we have that.
            // t.aztime  vector of time of the azimuth.  Don't think we have that really.  Will make it the radial set
            // t.x, y, z,  coordinates in the global grid
            // newvalue,
            // scaled_dist (short).   A Scale of the weight for the point:
            // 'quality' was passed into as RadialSet...ignoring it for now not sure it's used
            // RANGE_SCALE = 10
            // int(rangeMeters*RANGE_SCALE_KM*quality + 0.5);
            // int(rangeMeters*RANGE_SCALE_KM + 0.5);
            //    RANGE_SCALE_KM = RANGE_SCALE/1000.0
            // elevWeightScaled (char).
            // AZIMUTH_SCALE 100
            // ELEV_SCALE(x) = ( int (0.5+(x+90)*100) )
            // ELEV_UNSCALE(y) ( (y*0.01)-90)
            // ELEV_WT_SCALE 100
            // I think the stage 2 entries are gonna be difficult to match, there's information
            // that we can't include I think.  Stage 2 is trying to do things we already did?
            // Bleh maybe it will be good enough.  We'll need a better stage 2 I think.
            // FIXME: clean up so much here
            // entries.azimuth.push_back(int(0.5+azDegs*100)); // scale right?
            // RObsBinaryTable::mrmstime aTime;
            // FIXME: fill in time.  Ok what time do we use here?
            // entries.aztime.push_back(aTime); // scale right?
          }
        }
      }

      //    writeOutputProduct("NEWDEM", myDEM); // Write out modified dem if wanted?
      // WRITE LATLONGRID
      if (myWriteLLG) {
        std::map<std::string, std::string> myOverrides;
        myOverrides["postSuccessCommand"] = "ldm";
        writeOutputProduct("Mapped" + output->getTypeName(), output, myOverrides); // Typename will be replaced by -O filters
      }
      // WRITE LATLONGRID
      //
      double percentAttempt = (double) (attemptCount) / (double) (totalLayer) * 100.0;
      LogInfo("  Completed Layer " << vv.layerHeightKMs << " KMs. " << totalLayer << " left.\n");
      totalLayer -= rangeSkipped;
      LogInfo("    Range skipped: " << rangeSkipped << ". " << totalLayer << " left.\n");
      totalLayer -= terrainBlocked;
      LogInfo("    Terrain blocked (50%): " << terrainBlocked << ". " << totalLayer << " left.\n");
      totalLayer -= sameTiltSkip;
      LogInfo("    Same cached upper/lower skip: " << sameTiltSkip << ". " << totalLayer << " left.\n");
      LogInfo("    Attempting: " << attemptCount << " or " << percentAttempt << " %.\n");
      LogInfo("      Difference count (number sent to stage2): " << differenceCount << "\n");
      LogInfo("      _New_ upper/Lower good hits: " << upperGood << " and " << lowerGood << "\n");
      LogInfo("----------------------------------------\n");
    } // END HEIGHT LOOP

    // Changed entries write out
    // WRITE RAW
    if (!myWriteLLG) {                                          // only do one type for moment
      writeOutputProduct("Mapped" + r->getTypeName(), entries); // Typename will be replaced by -O filters
    }
    // writeOutputProduct(r->getTypeName(), entries); // Typename will be replaced by -O filters
    // WRITE RAW
    //
    LogInfo("Total all layer grid points: " << total << "\n");
    // writeOutputProduct("Mapped"+r->getTypeName(), entries); // Typename will be replaced by -O filters

    // ----------------------------------------------------------------------------
  }
} // RAPIOFusionOneAlg::processNewData

int
main(int argc, char * argv[])
{
  // FIXME: What if I want to use the system but without an algorithm?  Lol..
  // no I refuse to do a Baseline call...growl.  More work to do

  // Create and run a tile alg
  RAPIOFusionOneAlg alg = RAPIOFusionOneAlg();

  alg.executeFromArgs(argc, argv);
}
