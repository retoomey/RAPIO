#include "rFusion1.h"
#include "rBinaryTable.h"
#include "rVolumeValueResolver.h"
#include "rConfigRadarInfo.h"

using namespace rapio;

// Volume value resolver (alphas)

class RobertLinear1Resolver : public VolumeValueResolver
{
public:

  /** Introduce into VolumeValueResolver factory */
  static void
  introduceSelf()
  {
    std::shared_ptr<RobertLinear1Resolver> newOne = std::make_shared<RobertLinear1Resolver>();
    Factory<VolumeValueResolver>::introduce("robert", newOne);
  }

  /** Create by factory */
  virtual std::shared_ptr<VolumeValueResolver>
  create(const std::string & params)
  {
    return std::make_shared<RobertLinear1Resolver>();
  }

  virtual void
  calc(VolumeValue& vv) override
  {
    // ------------------------------------------------------------------------------
    // Query information for above and below the location
    bool haveLower = queryLayer(vv, vv.lower, vv.lLayer);
    bool haveUpper = queryLayer(vv, vv.upper, vv.uLayer);

    // ------------------------------------------------------------------------------
    // In beamwidth calculation
    //
    // FIXME: might be able to optimize by not always calculating in beam width,
    // but it's making my head hurt so just do it always for now
    // The issue is between tilts lots of tan, etc. we might skip

    // Get height of half beamwidth higher
    LengthKMs lowerHeightKMs;
    bool inLowerBeamwidth = false;

    if (haveLower) { // Do we hit the valid gates of the lower tilt?
      heightForDegreeShift(vv, vv.lower, vv.lLayer.beamWidth / 2.0, lowerHeightKMs);
      inLowerBeamwidth = (vv.layerHeightKMs <= lowerHeightKMs);
    }

    // Get height of half beamwidth lower
    LengthKMs upperHeightKMs;
    bool inUpperBeamwidth = false;

    if (haveUpper) { // Do we hit the valid gates of the upper tilt?
      heightForDegreeShift(vv, vv.upper, -vv.uLayer.beamWidth / 2.0, upperHeightKMs);
      inUpperBeamwidth = (vv.layerHeightKMs >= upperHeightKMs);
    }

    // ------------------------------------------------------------------------------
    // Background MASK calculation
    //
    // Keep background mask logic separate for now, though would probably be faster
    // doing it in the value calculation.  I recommend doing it this way because
    // logically it frys your brain a bit less.  You can 'have' an upper tilt,
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
    // Make values unavailable if cumulative blockage is over 50%
    // FIXME: Could be configurable
    if (vv.uLayer.terrainCBBPercent > .50) {
      vv.uLayer.value = Constants::DataUnavailable;
    }
    if (vv.lLayer.terrainCBBPercent > .50) {
      vv.lLayer.value = Constants::DataUnavailable;
    }
    // Make values unavailable if elevation layer bottom hits terrain
    if (vv.lLayer.beamHitBottom) {
      vv.lLayer.value = Constants::DataUnavailable;
    }
    if (vv.uLayer.beamHitBottom) {
      vv.uLayer.value = Constants::DataUnavailable;
    }


    // ------------------------------------------------------------------------------
    // Interpolation and application of upper and lower data values
    //
    const double& lValue = vv.lLayer.value;
    const double& uValue = vv.uLayer.value;

    if (Constants::isGood(lValue) && Constants::isGood(uValue)) {
      // Linear interpolate using heights.  With two values we can do interpolation
      // between the values always, either linear or exponential
      double wt = (vv.layerHeightKMs - vv.lLayer.heightKMs) / (upperHeightKMs - vv.uLayer.heightKMs);
      if (wt < 0) { wt = 0; } else if (wt > 1) { wt = 1; }
      const double nwt = (1.0 - wt);

      const double lTerrain = lValue * (1 - vv.lLayer.terrainCBBPercent);
      const double uTerrain = uValue * (1 - vv.uLayer.terrainCBBPercent);

      // v = (0.5 + nwt * lValue + wt * uValue);
      v = (0.5 + nwt * lTerrain + wt * uTerrain);
    } else if (inLowerBeamwidth) {
      if (Constants::isGood(lValue)) {
        const double lTerrain = lValue * (1 - vv.lLayer.terrainCBBPercent);
        v = lTerrain;
      } else {
        v = lValue; // Use the gate value even if missing, RF, unavailable, etc.
      }
    } else if (inUpperBeamwidth) {
      if (Constants::isGood(uValue)) {
        const double uTerrain = uValue * (1 - vv.uLayer.terrainCBBPercent);
        v = uTerrain;
      } else {
        v = uValue;
      }
    }

    vv.dataValue = v;
  } // calc
};

/**
 * A resolver attempting to use the math presented in the paper:
 * "A Real-Time, Three-Dimensional, Rapidly Updating, Heterogeneous Radar
 * Merger Technique for Reflectivity, Velocity, and Derived Products"
 * Weather and Forecasting October 2006
 * Valliappa Lakshmanan, Travis Smith, Kurt Hondl, Greg Stumpf
 *
 * In particular, page 10 describing Virtual Volumes and the elevation
 * influence.
 */
class LakResolver1 : public VolumeValueResolver
{
public:

  /** Introduce into VolumeValueResolver factory */
  static void
  introduceSelf()
  {
    std::shared_ptr<LakResolver1> newOne = std::make_shared<LakResolver1>();
    Factory<VolumeValueResolver>::introduce("lak", newOne);
  }

  /** Create by factory */
  virtual std::shared_ptr<VolumeValueResolver>
  create(const std::string & params)
  {
    return std::make_shared<LakResolver1>();
  }

  virtual void
  calc(VolumeValue& vv) override
  {
    double v = Constants::DataUnavailable; // Final output data value

    static const float ELEV_FACTOR     = log(0.005); // Formula constant from paper
    static const float TERRAIN_PERCENT = .50;        // Cutoff terrain cumulative blockage
    static const float MAX_SPREAD_DEGS = 4;          // Max spread of degrees between tilts
    static const float ELEV_THRESH     = .53;        // Closest to current w2merger

    // ------------------------------------------------------------------------------
    // Query information for above and below the location
    bool haveLower       = queryLayer(vv, vv.lower, vv.lLayer);
    bool haveUpper       = queryLayer(vv, vv.upper, vv.uLayer);
    const double& lValue = vv.lLayer.value;
    const double& uValue = vv.uLayer.value;

    // Weighted average use
    bool useLower  = false;
    bool useUpper  = false;
    double upperWt = 0.0;
    double lowerWt = 0.0;

    // Discard tilts/values that hit terrain
    if ((vv.uLayer.terrainCBBPercent > TERRAIN_PERCENT) || (vv.uLayer.beamHitBottom)) {
      haveUpper = false;
    }
    if ((vv.lLayer.terrainCBBPercent > TERRAIN_PERCENT) || (vv.lLayer.beamHitBottom)) {
      haveLower = false;
    }

    // Still trying to get good results before optimizing

    // Experimental function to extrapolate from lower to upper
    // FIXME: if works, both sections of code could share function/macro
    if (haveLower) {
      // Formula (6) on page 10 ----------------------------------------------
      const double alphaTop = vv.virtualElevDegs - vv.lLayer.elevation;

      double alphaBottom = vv.lLayer.beamWidth;
      if (haveUpper) {
        const double spreadDegs = vv.uLayer.elevation - vv.lLayer.elevation;
        if (spreadDegs <= MAX_SPREAD_DEGS) { // if in the spread range, use the spread
          if (spreadDegs > alphaBottom) { alphaBottom = spreadDegs; }
        }
      }

      const double alpha = alphaTop / alphaBottom;
      double delta       = exp(alpha * alpha * alpha * ELEV_FACTOR);
      // ---------------------------------------------------------------------

      // Paper: It can be seen that where the 'delta' is less than 0.5, the voxel
      // is outside the effective beamwidth of the radial
      if (alpha <= ELEV_THRESH) { // Fall off exp(0.125*ln(0.005)) == 0.5156692688
        if (Constants::isGood(lValue)) {
          // Don't do this: if (delta < 0) { delta = 0; } else if (delta > 1) { delta = 1; }
          //  v = lValue; //  * delta * (1.0 - vv.lLayer.terrainCBBPercent);
          lowerWt  = delta;
          useLower = true;
        } else {
          v = Constants::MissingData; // mask
        }
      }
    }

    if (haveUpper) {
      // Formula (6) on page 10 ----------------------------------------------
      const double alphaTop = vv.uLayer.elevation - vv.virtualElevDegs;

      double alphaBottom = vv.uLayer.beamWidth;
      if (haveLower) {
        const double spreadDegs = vv.uLayer.elevation - vv.lLayer.elevation;
        if (spreadDegs <= MAX_SPREAD_DEGS) { // if in the spread range, use the spread
          if (spreadDegs > alphaBottom) { alphaBottom = spreadDegs; }
        }
      }

      const double alpha = alphaTop / alphaBottom;
      double delta       = exp(alpha * alpha * alpha * ELEV_FACTOR);
      // ---------------------------------------------------------------------

      // Paper: It can be seen that where the 'delta' is less than 0.5, the voxel
      // is outside the effective beamwidth of the radial
      if (alpha <= ELEV_THRESH) { // Fall off exp(0.125*ln(0.005)) == 0.5156692688
        if (Constants::isGood(uValue)) {
          // Don't do this: if (delta < 0) { delta = 0; } else if (delta > 1) { delta = 1; }
          // v = uValue * delta * (1.0 - vv.uLayer.terrainCBBPercent);
          upperWt  = delta;
          useUpper = true;
        } else {
          v = Constants::MissingData; // mask
        }
      }
    }

    // Weighted average.  Do we bother separating?
    // For moment I want explicit cases for testing/debugging
    if (useLower) { // 10, 11
      if (useUpper) {
        // weighted average both (might be quicker to just do this vs branching)
        v = ((upperWt * uValue) + (lowerWt * lValue)) / (upperWt + lowerWt);
        // std::cout << "merged: " << upperWt << ", " << uValue << ", " << lowerWt << ", " << lValue << ", " << (uValue+lValue) << " ==== " << v << "\n";
      } else {
        v = lValue; // weighted only with self
        // std::cout << "lower: " << lValue << " ("<<lowerWt << ")\n";
      }
    } else {
      if (useUpper) { // 01
        v = uValue;   // weighted only with self
        // std::cout << "upper: " << uValue << " ("<<upperWt << ")\n";
      }
    }

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

  // FIXME: Thinking general new plugin class in API here.  All three of these
  // add similar abilities

  // Volume value resolver
  o.optional("resolver", "lak",
    "Value Resolver Algorithm, such as 'lak', or your own. Params follow: lak,params.");
  o.addAdvancedHelp("resolver",
    "Value Resolver algorithms are registered by name, so you can add you own options here with this option.");

  // Elevation volume
  o.optional("volume", "simple",
    "Volume algorithm, such as 'simple', or your own. Params follow: simple,params.");
  o.addAdvancedHelp("volume",
    "Volume algorithms are registered by name, so you can add you own options here with this option.  The default 'simple' algorithm uses a virtual volume that replaces incoming elevation angles by time.");

  // Terrain blockage name
  o.optional("terrain", "2me",
    "Terrain blockage algorithm, such as 'lak', '2me', or your own. Params follow: lak,/DEMS.");
  o.addAdvancedHelp("terrain",
    "Terrain blockage algorithms are registered by name, so you can add your own options here with this option.  You have some general param support in the form 'key,params' where the params string is passed onto your terrain blockage instance. For example, the lak and 2me terrain algorithms want a DEM file of the form RADARNAME.nc The path for this can be given as 'lak,/MYDEMS' or '2me,/MYDEMS'.");

  // Range to use in KMs.  Default is 460.  This determines subgrid and max range of valid
  // data for the radar
  o.optional("rangekm", "460", "Range in kilometers for radar.");
} // RAPIOFusionOneAlg::declareOptions

/** RAPIOAlgorithms process options on start up */
void
RAPIOFusionOneAlg::processOptions(RAPIOOptions& o)
{
  // Get grid
  o.getLegacyGrid(myFullGrid);

  // Still keeping the myHeightsIndex 'hack' which allows us
  // to take a subset of the heights within full conus space,
  // but at some point we can probably get rid of it
  #if 1
  myHeightsM = myFullGrid.heightsM;
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

  myWriteLLG    = o.getBoolean("llg");
  myResolverAlg = o.getString("resolver");
  myTerrainAlg  = o.getString("terrain");
  myVolumeAlg   = o.getString("volume");
  myRangeKMs    = o.getFloat("rangekm");
  if (myRangeKMs < 50) {
    myRangeKMs = 50;
  } else if (myRangeKMs > 1000) {
    myRangeKMs = 1000;
  }
  LogInfo("Range range is " << myRangeKMs << " Kilometers\n");
} // RAPIOFusionOneAlg::processOptions

std::shared_ptr<LatLonGrid>
RAPIOFusionOneAlg::createLLG(
  const std::string   & outputName,
  const std::string   & outputUnits,
  const LLCoverageArea& g,
  const LengthKMs     layerHeightKMs)
{
  // Create a new working space LatLonGrid for the layer
  auto output = LatLonGrid::
    Create(outputName, outputUnits, LLH(g.nwLat, g.nwLon, layerHeightKMs), Time(), g.latSpacing, g.lonSpacing,
      g.numY, g.numX);

  // Extra setup for any of our LLG objects

  output->setReadFactory("netcdf"); // default to netcdf output if we write it

  // Default the LatLonGrid to DataUnvailable, we'll fill in good values later
  auto array = output->getFloat2D();

  array->fill(Constants::DataUnavailable);

  return output;
}

void
RAPIOFusionOneAlg::createLLGCache(
  const std::string        & outputName,
  const std::string        & outputUnits,
  const LLCoverageArea     & g,
  const std::vector<double>& heightsM)
{
  // NOTE: Tried it as a 3D array.  Due to memory fetching, etc. having N 2D arrays
  // turns out to be faster than 1 3D array.
  if (myLLGCache.size() == 0) {
    ProcessTimer("Creating initial LLG value cache.");

    // For each of our height layers to process
    for (size_t layer = 0; layer < heightsM.size(); layer++) {
      const LengthKMs layerHeightKMs = heightsM[layer] / 1000.0;
      myLLGCache.add(createLLG(outputName, outputUnits, g, layerHeightKMs));
    }
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
    mySinCosCache = std::make_shared<SinCosLatLonCache>(g.numX, g.numY);

    // LogInfo("Projection LatLonHeight to AzRangeElev cache creation...\n");
    for (size_t i = 0; i < myLLProjections.size(); i++) {
      const LengthKMs layerHeightKMs = myHeightsM[i] / 1000.0;
      LogInfo("  Layer: " << myHeightsM[i] << " meters.\n");
      myLLProjections[i] = std::make_shared<AzRanElevCache>(g.numX, g.numY);
      auto& llp = *(myLLProjections[i]);
      auto& ssc = *(mySinCosCache);
      AngleDegs startLat = g.nwLat - (g.latSpacing / 2.0); // move south (lat decreasing)
      AngleDegs startLon = g.nwLon + (g.lonSpacing / 2.0); // move east (lon increasing)
      AngleDegs atLat    = startLat;

      llp.reset();
      ssc.reset();

      for (size_t y = 0; y < g.numY; y++, atLat -= g.latSpacing) { // a north to south
        AngleDegs atLon = startLon;
        for (size_t x = 0; x < g.numX; x++, atLon += g.lonSpacing) { // a east to west row, changing lon per cell
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

namespace {
/** Split a param string into key,somestring */
void
splitKeyParam(const std::string& commandline, std::string& key, std::string& params)
{
  std::vector<std::string> twoparams;

  Strings::splitOnFirst(commandline, ',', &twoparams);
  if (twoparams.size() > 1) {
    key    = twoparams[0];
    params = twoparams[1];
  } else {
    key    = commandline;
    params = "";
  }
}
}

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
    RObsBinaryTable::mrmstime aMRMSTime;
    aMRMSTime.epoch_sec = rTime.getSecondsSinceEpoch();
    aMRMSTime.frac_sec  = rTime.getFractional();
    // Radar center coordinates
    const LLH center        = r->getRadarLocation();
    const AngleDegs cLat    = center.getLatitudeDeg();
    const AngleDegs cLon    = center.getLongitudeDeg();
    const LengthKMs cHeight = center.getHeightKM();

    // Get the radar name and typename from this RadialSet.
    std::string name = "UNKNOWN";
    const std::string aTypeName = r->getTypeName();
    const std::string aUnits    = r->getUnits();
    if (!r->getString("radarName-value", name)) {
      LogSevere("No radar name found in RadialSet, ignoring data\n");
      return;
    }
    ;

    LogInfo(
      " " << r->getTypeName() << " (" << r->getNumRadials() << " * " << r->getNumGates() << ")  Radials * Gates,  Time: " << r->getTime() <<
        "\n");

    // ----------------------------------------------------------------------------
    // Every RADAR/MOMENT will require a elevation volume for virtual volume
    // This is a time expiring auto sorted volume based on elevation angle (subtype)
    // FIXME: Move this to own method when stable
    static bool setup = false;

    static std::shared_ptr<VolumeValueResolver> resolversp = nullptr;

    if (!setup) {
      setup = true;

      // Link to first incoming radar and moment, we will ignore any others from now on
      LogInfo(
        "Linking this algorithm to radar '" << name << "' and typename '" << aTypeName <<
          "' since first pass we only handle 1\n");
      myTypeName  = aTypeName;
      myRadarName = name;

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
      // VolumeValueResolver registration and creation
      VolumeValueResolver::introduceSelf();
      RobertLinear1Resolver::introduceSelf();
      LakResolver1::introduceSelf();

      splitKeyParam(myResolverAlg, key, params);
      resolversp = VolumeValueResolver::createVolumeValueResolver(key, params);

      // Stubbornly refuse to run if Volume Value Resolver requested by name and not found or failed
      if (resolversp == nullptr) {
        LogSevere("Volume Value Resolver '" << key << "' requested, but failed to find and/or initialize.\n");
        exit(1);
      } else {
        LogInfo("Using Volume Value Resolver algorithm '" << key << "'\n");
      }
      // VolumeValueResolver::introduce("yourresolver", myResolverClass); To add your own

      // -------------------------------------------------------------
      // Terrain blockage registration and creation
      TerrainBlockage::introduceSelf();
      // TerrainBlockage::introduce("yourterrain", myTerrainClass); To add your own

      // Set up generic params for volume and terrain blockage classes
      // in form "--optionname=key,somestring"
      // We want a 'key' always to choose the algorithm, the rest is passed to the
      // particular terrain algorithm to use as it wishes.
      // The Lak and 2me ones take a DEM folder as somestring
      splitKeyParam(myTerrainAlg, key, params);
      myTerrainBlockage = TerrainBlockage::createTerrainBlockage(key, params,
          r->getLocation(), myRangeKMs, name);

      // Stubbornly refuse to run if terrain requested by name and not found or failed
      if (myTerrainBlockage == nullptr) {
        LogSevere("Terrain blockage '" << key << "' requested, but failed to find and/or initialize.\n");
        exit(1);
      } else {
        LogInfo("Using TerrainBlockage algorithm '" << key << "'\n");
      }

      // -------------------------------------------------------------
      // Elevation volume registration and creation
      LogInfo(
        "Creating virtual volume for '" << myRadarName << "' and typename '" << myTypeName <<
          "'\n");
      Volume::introduceSelf();
      // Volume::introduce("yourvolume", myVolumeClass); To add your own

      splitKeyParam(myVolumeAlg, key, params);
      myElevationVolume = Volume::createVolume(key, params, myRadarName + "_" + myTypeName);

      // Stubbornly refuse to run if terrain requested by name and not found or failed
      if (myElevationVolume == nullptr) {
        LogSevere("Volume '" << key << "' requested, but failed to find and/or initialize.\n");
        exit(1);
      } else {
        LogInfo("Using Volume algorithm '" << key << "'\n");
      }

      // Look up from cells to az/range/elev for RADAR
      createLLHtoAzRangeElevProjection(cLat, cLon, cHeight, outg);

      // Generate output name and units
      const std::string outputName = r->getTypeName() + "Fused";
      std::string outputUnits      = r->getUnits();
      if (outputUnits.empty()) {
        LogSevere("Units still wonky because of W2 bug, forcing dBZ at moment..\n");
        outputUnits = "dBZ";
      }

      // Create a single 2D covering full grid (for outputting full vs subgrid files)
      // The web page, etc. require full conus referenced data.
      // We don't worry about height, we'll set it for each layer
      if (myWriteLLG) {
        myFullLLG = createLLG(outputName, outputUnits, myFullGrid, 0);
      }

      // Create working LLG cache CAPPI storage per height level
      createLLGCache(outputName, outputUnits, outg, myHeightsM);
    }
    auto& resolver = *resolversp;

    // Check if incoming radar/moment matches our single setup, otherwise we'd need
    // all the setup for each radar/moment.  Which we 'could' do later maybe
    if ((myRadarName != name) || (myTypeName != aTypeName)) {
      LogSevere(
        "We are linked to '" << myRadarName << "-" << myTypeName << "', ignoring radar/typename '" << name << "-" << aTypeName <<
          "'\n");
      return;
    }

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
    // RAW file entry storage
    //
    // Entries to write out eventually
    std::shared_ptr<RObsBinaryTable> entries = std::make_shared<RObsBinaryTable>();
    entries->setReadFactory("raw"); // default to raw output if we write it
    auto& t = *entries;

    // FIXME: refactor/methods, etc. This isn't clean by a long shot
    t.radarName = myRadarName;
    t.vcp       = -9999; // Ok will merger be ok with this, need to check
    t.elev      = elevDegs;
    t.typeName  = aTypeName;
    t.setUnits(aUnits);
    t.markedLinesCacheFile = ""; // We ignore marked lines.  We'll see how it goes...
    t.lat       = cLat;          // lat degrees
    t.lon       = cLon;          // lon degrees
    t.ht        = cHeight;       // kilometers assumed by w2merger
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

    // Get the elevation volume pointers and levels for speed
    std::vector<double> levels;
    std::vector<DataType *> pointers;
    myElevationVolume->getTempPointerVector(levels, pointers);
    LogInfo(*myElevationVolume << "\n");
    // F(Lat,Lat,Height) --> Virtual Az, Elev, Range projection add spacing/2 to get cell cellcenters
    AngleDegs startLat = myRadarGrid.nwLat - (myRadarGrid.latSpacing / 2.0); // move south (lat decreasing)
    AngleDegs startLon = myRadarGrid.nwLon + (myRadarGrid.lonSpacing / 2.0); // move east (lon increasing)

    // Each layer of merger we have to loop through
    LLCoverageArea outg = myRadarGrid;
    size_t total        = 0;
    for (size_t layer = 0; layer < myHeightsM.size(); layer++) {
      vv.layerHeightKMs = myHeightsM[layer] / 1000.0;

      // FIXME: Do we put these into the output?
      size_t totalLayer   = 0;
      size_t rangeSkipped = 0; // Skip by range (outside circle)
      size_t upperGood    = 0; // Have tilt above
      size_t lowerGood    = 0; // Have tilt below
      size_t sameTiltSkip = 0;

      size_t attemptCount    = 0;
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
          myElevationVolume->getSpreadL(vv.virtualElevDegs, levels, pointers, vv.lower, vv.upper);
          if (vv.lower != nullptr) { lowerGood++; }
          if (vv.upper != nullptr) { upperGood++; }

          // If the upper and lower for this point are the _same_ RadialSets as before, then
          // the interpolation will end up with the same data value.
          // This takes advantage that in a volume if a couple tilts change, then
          // the space time affected is fairly small compared to the entire volume's 3D coverage.

          // I'm gonna start with the pointers as keys, which are numbers, but it's 'possible'
          // a new radialset could get same pointer (expire/new).
          // FIXME: Probably super rare to get same pointer, but maybe we add
          // a RadialSet or general DataType counter at some point to be sure.
          // Humm.  The tilt time 'might' work appending it to the pointer value.
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
              int(vv.virtualRangeKMs * RANGE_SCALE + 0.5),
              //	  entry.elevWeightScaled(), Can't figure this out.  Why a individual scale...oh weight because the
              //	  cloud is the full cube.  Ok so we take current elev..no wait the virtual elevation and scale to char...
              //	  takes -90 to 90 to 0-18000, can still fit into unsigned char...
              //
              ELEV_SCALE(vv.virtualElevDegs),
              vv.virtualAzDegs, aMRMSTime);

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

      // --------------------------------------------------------
      // Write LatLonGrid or Raw merger files (FIXME: Maybe more flag control)
      //

      // Try pushing to ldm afterwards. FIXME: Probably need a command flag
      std::map<std::string, std::string> myOverrides;

      //  FIXME: hidden flag for ldm writing myOverrides["postSuccessCommand"] = "ldm";
      myOverrides["postSuccessCommand"] = "ldm";
      if (myWriteLLG) {
        // Typename will be replaced by -O filters

        // Subgrid file
        // writeOutputProduct("Mapped" + output->getTypeName(), output, myOverrides);

        // Copy the current output subgrid into the full LLG for writing out...
        // Note that since the 2D coordinates don't change, any 'outside' part doesn't need
        // to be written and can stay DataUnavailable
        // FIXME: Could maybe be a copy method in LatLonGrid with multiple coverage areas
        auto& fullgrid = myFullLLG->getFloat2DRef();
        auto& subgrid  = output->getFloat2DRef();

        // Sync height and time with the current output grid
        // FIXME: a getHeight/setHeight?  Seems silly to pull/push here so much
        myFullLLG->setTime(r->getTime());
        auto fullLoc = myFullLLG->getLocation();
        auto subLoc  = output->getLocation();
        fullLoc.setHeightKM(subLoc.getHeightKM());
        myFullLLG->setLocation(fullLoc);

        // Copy subgrid into the full grid
        size_t startY = outg.startY;
        size_t startX = outg.startX;
        for (size_t y = 0; y < outg.numY; y++) {   // a north to south
          for (size_t x = 0; x < outg.numX; x++) { // a east to west row
            fullgrid[startY + y][startX + x] = subgrid[y][x];
          }
        }

        // Full grid file (CONUS for regular merger)
        writeOutputProduct("Mapped" + myFullLLG->getTypeName(), myFullLLG, myOverrides);
      } else {
        // WRITE RAW
        writeOutputProduct("Mapped" + r->getTypeName(), entries, myOverrides);
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
    } // END HEIGHT LOOP

    LogInfo("Total all layer grid points: " << total << "\n");

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
