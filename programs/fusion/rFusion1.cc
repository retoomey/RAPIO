#include "rFusion1.h"
#include "rBinaryTable.h"
#include "rVolumeValueResolver.h"
#include "rConfigRadarInfo.h"

using namespace rapio;

// Some test resolvers for sanity checks...

/** Azimuth from 0 to 360 or so of virtual */
class AzimuthVVResolver : public VolumeValueResolver
{
public:
  virtual void
  calc(VolumeValue& vv) override
  {
    // Output value is the virtual azimuth
    vv.dataValue = vv.virtualAzDegs;
  }
};

/** Virtual range in KMs */
class RangeVVResolver : public VolumeValueResolver
{
public:
  virtual void
  calc(VolumeValue& vv) override
  {
    // Output value is the virtual range
    vv.dataValue = vv.virtualRangeKMs;
  }
};

/** Projected terrain blockage of lower tilt.
 * Experiment to display some of the terrain fields for debugging */
class TerrainVVResolver : public VolumeValueResolver
{
public:
  virtual void
  calc(VolumeValue& vv) override
  {
    bool haveLower = queryLayer(vv, vv.lower, vv.lLayer);

    // vv.dataValue = vv.lLayer.beamHitBottom ? 1.0: 0.0;
    // vv.dataValue = vv.lLayer.terrainPBBPercent;
    if (vv.lLayer.beamHitBottom) {
      // beam bottom on terrain we'll plot as unavailable
      vv.dataValue = Constants::DataUnavailable;
    } else {
      vv.dataValue = vv.lLayer.terrainCBBPercent;
      // Super small we'll go unavailable...
      if (vv.dataValue < 0.02) {
        vv.dataValue = Constants::MissingData;
      } else {
        // Otherwise scale a bit to show up with colormap better
        // 0 to 10000
        vv.dataValue *= 100;
        vv.dataValue  = vv.dataValue * vv.dataValue;
      }
    }
  }
};


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

/** First attempt to do the wt^3 Lak does in w2merger */
class RobertGuassian1Resolver : public VolumeValueResolver
{
public:
  virtual void
  calc(VolumeValue& vv) override
  {
    // ------------------------------------------------------------------------------
    // Query information for above and below the location
    bool haveLower       = queryLayer(vv, vv.lower, vv.lLayer);
    bool haveUpper       = queryLayer(vv, vv.upper, vv.uLayer);
    const double& lValue = vv.lLayer.value;
    const double& uValue = vv.uLayer.value;

    static const float ELEV_FACTOR = log(0.005);

    // Final value
    double v = Constants::DataUnavailable;

    // Do Lak's exp curve extrapolation from the upper to our sample location...
    // Lak would cap this at full beamwidth, I think to get half the exponential curve
    // Also we'll do angle interpolation here vs the linear though I might try that
    // later.
    double upperWt = 0.0;
    double lowerWt = 0.0;

    if (haveUpper) {
      if ((vv.uLayer.terrainCBBPercent > .50) || (vv.uLayer.beamHitBottom)) {
        haveUpper = false;
      } else {
        // Get coverage over the beamwidth, half the final S curve
        const double c = (vv.uLayer.elevation - vv.virtualElevDegs) / vv.uLayer.beamWidth;
        if (c < 1) { v = Constants::MissingData; } // in beamwidth mask

        // If the value is a good one, do more weight math and update
        if (Constants::isGood(uValue)) {
          double wt = exp(c * c * c * ELEV_FACTOR);
          if (wt < 0) { wt = 0; } else if (wt > 1) { wt = 1; }
          upperWt = wt;
        } else {
          haveUpper = false; // ignore bad values as if they don't exist
        }
      }
    }

    if (haveLower) {
      if ((vv.lLayer.terrainCBBPercent > .50) || (vv.lLayer.beamHitBottom)) {
        haveLower = false;
      } else {
        // Get coverage over the beamwidth, half the final S curve
        const double c = (vv.virtualElevDegs - vv.lLayer.elevation ) / vv.lLayer.beamWidth;
        if (c < 1) { v = Constants::MissingData; } // in beamwidth mask

        // If the value is a good one, do more weight math and update
        if (Constants::isGood(lValue)) {
          double wt = exp(c * c * c * ELEV_FACTOR);
          if (wt < 0) { wt = 0; } else if (wt > 1) { wt = 1; }
          lowerWt = wt;
        } else {
          haveLower = false; // ignore bad values as if they don't exist
        }
      }
    }

    if (haveLower && haveUpper) {
      // Combine the two into one
      v = ((upperWt * uValue) + (lowerWt * lValue)) / (upperWt + lowerWt);
    } else if (haveUpper) {
      v = upperWt * uValue;
    } else if (haveLower) {
      v = lowerWt * lValue;
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

  // Terrain DEM location folder with files of form RADAR.nc
  // Algorithm will stop if terrain cannot be found here for radar (for non-blank)
  o.optional("terrain", "", "Path to terrain files of form RADAR.nc, otherwise we ignore terrain clipping.");

  // Terrain blockage name
  o.optional("terrainalg", "2me", "Registered terrain blockage algorithm to use, such as 'lak', '2me', or your own.");

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

  myWriteLLG    = o.getBoolean("llg");
  myTerrainPath = o.getString("terrain");
  myTerrainAlg  = o.getString("terrainalg");
  myRangeKMs    = o.getFloat("rangekm");
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
  // NOTE: Tried it as a 3D array.  Due to memory fetching, etc. having N 2D arrays
  // turns out to be faster than 1 3D array.
  // FIXME: We could have a LatLonHeightGrid implementation/create to generalize this
  if (myLLGCache.size() == 0) {
    ProcessTimer("Creating initial LLG value cache.");
    const std::string aName = r->getTypeName() + "Fused";
    std::string aUnits      = r->getUnits();
    if (aUnits.empty()) {
      LogSevere("Units still wonky because of W2 bug, forcing dBZ at moment..\n");
      aUnits = "dBZ";
    }

    // For each of our height layers to process
    for (size_t layer = 0; layer < heightsM.size(); layer++) {
      const LengthKMs layerHeightKMs = heightsM[layer] / 1000.0;

      // Create a new working space LatLonGrid for the layer
      auto output = LatLonGrid::
        Create(aName, aUnits, LLH(g.nwLat, g.nwLon, layerHeightKMs), Time(), g.latSpacing, g.lonSpacing,
          g.numY, g.numX);
      output->setReadFactory("netcdf"); // default to netcdf output if we write it

      // Default the LatLonGrid to DataUnvailable, we'll fill in good values later
      auto array = output->getFloat2D();
      array->fill(Constants::DataUnavailable);

      myLLGCache.add(output);
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

    LogInfo(
      " " << r->getTypeName() << " (" << r->getNumRadials() << " * " << r->getNumGates() << ")  Radials * Gates,  Time: " << r->getTime() <<
        "\n");

    // ----------------------------------------------------------------------------
    // For the moment, we can only be linked to one radar and one typename
    // such as reflectivity

    // Get the radar name and typename from this RadialSet.
    std::string name = "UNKNOWN";
    const std::string aTypeName = r->getTypeName();
    const std::string aUnits    = r->getUnits();
    if (r->getString("radarName-value", name)) {
      // LogInfo("      Radar name is found!  It is  '" << name << "'\n");
      if (myRadarName.empty()) {
        LogInfo(
          "Linking this algorithm to radar '" << name << "' and typename '" << aTypeName <<
            "' since first pass we only handle 1\n");
        myTypeName  = aTypeName;
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
    // This is a time expiring auto sorted volume based on elevation angle (subtype)
    //
    if (myElevationVolume == nullptr) {
      LogInfo(
        "Creating virtual volume and terrain blockage for '" << myRadarName << "' and typename '" << myTypeName <<
          "'\n");
      myElevationVolume = std::make_shared<ElevationVolume>(myRadarName + "_" + myTypeName);

      // Terrain blockage algorithm is a lookup with a given range and density
      myTerrainBlockage = nullptr;
      if (myDEM != nullptr) {
        // We want the default terrain blockage algorithms.
        static bool setupTerrain = false;
        if (!setupTerrain) {
          // Register the default TerrainBlockage objects
          TerrainBlockage::introduceSelf();
          setupTerrain = true;
        }
        // TerrainBlockage::introduce("yourterrain", myterrainclass);

        myTerrainBlockage = TerrainBlockage::createTerrainBlockage(myTerrainAlg,
            myDEM, r->getLocation(), myRangeKMs, name);
      }
      // Stubbornly refuse to run if terrain requested by name and not found
      if (myTerrainBlockage == nullptr) {
        exit(1);
      } else {
        LogInfo("Using TerrainBlockage algorithm '" << myTerrainAlg << "'\n");
      }
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

    // ----------------------------------------------------------------------------
    // We inset the full merger grid to the subgrid covered by the radar.
    // This is needed for massively large grids like the CONUS.
    //
    static bool firstTime = true;
    if (firstTime) {
      const LengthKMs fudgeKMs = 5; // Extra to include so circle is inside box a bit
      myRadarGrid = myFullGrid.insetRadarRange(cLat, cLon, myRangeKMs + fudgeKMs);
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
    createLLHtoAzRangeElevProjection(cLat, cLon, cHeight, outg);

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
    t.vcp       = -9999; // Ok will merger be ok with this, need to check
    t.elev      = elevDegs;
    t.typeName  = r->getTypeName();
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

    // Resolver
    // RobertLinear1Resolver resolver;
    RobertGuassian1Resolver resolver;
    // TestResolver1 resolver;
    // RangeVVResolver resolver;
    // TerrainVVResolver resolver;

    // LengthKMs rangeKMs;

    // Each layer of merger we have to loop through
    size_t total = 0;
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
      myOverrides["postSuccessCommand"] = "ldm";
      if (myWriteLLG) {
        std::map<std::string, std::string> myOverrides;
        // Typename will be replaced by -O filters
        writeOutputProduct("Mapped" + output->getTypeName(), output, myOverrides);
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
