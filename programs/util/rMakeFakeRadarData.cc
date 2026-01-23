#include "rMakeFakeRadarData.h"
#include "rTerrainBlockageLak.h"

using namespace rapio;

void
MakeFakeRadarData::declareOptions(RAPIOOptions& o)
{
  o.setDescription("MRMS Make fake radar data");
  o.setAuthors("Valliappa Lakshman, Robert Toomey");

  // Using same option letters as MRMS in an attempt not to confuse,
  // though better naming would be good.

  // Require the following
  o.require("s", "KTLX", "Radarname such as 'KTLX'.");

  // Optional stuff
  o.optional("T", "", "Terrain MRMS netcdf file to use such as '/reference/KTLX.nc'.");
  o.optional("w", "1.0", "Beamwidth degrees.");
  o.optional("S", "1.0", "Azimuthal spacing degrees."); // Differs from MRMS since this is explicit
  o.optional("g", "1000", "Gatewidth in meters.");
  o.optional("R", "300", "Radar range in kilometers.");
  o.optional("v", "30", "Initial fake data value before terrain application.");
  o.optional("chart", "",
    "Create DataGrid output for creating a python chart.  Currently angle for terrain chart only");
}

void
MakeFakeRadarData::processOptions(RAPIOOptions& o)
{
  myRadarName     = o.getString("s");
  myTerrainPath   = o.getString("T");
  myBeamWidthDegs = o.getFloat("w");
  myAzimuthalDegs = o.getFloat("S");
  myGateWidthM    = o.getFloat("g");
  myRadarRangeKM  = o.getFloat("R");
  myRadarValue    = o.getFloat("v");
  myChart         = o.getString("chart");

  // Get terrain information
  readTerrain();

  // Extra validation/calculation
  myNumGates   = int(0.5 + (1000.0 * myRadarRangeKM) / myGateWidthM);
  myNumRadials = 360.0 / myAzimuthalDegs;

  if (myDEM == nullptr) {
    fLogSevere("Ok you need to actually have terrain for this first alpha");
    exit(1);
  }
}

void
MakeFakeRadarData::execute()
{
  fLogInfo("Radarname: {}", myRadarName);
  fLogInfo("BeamWidth: {}", myBeamWidthDegs);
  fLogInfo("GateWidth: {}", myGateWidthM);
  fLogInfo("Radar range: {}", myRadarRangeKM);
  fLogInfo("Azimuthal spacing: {}", myAzimuthalDegs);

  // We don't have vcp so what to do.  FIXME: probably new parameters right?
  // FIXME: have vcp lists?  Bleh we got into trouble because of that in first place
  // AngleDegs elevAngleDegs = .50;
  AngleDegs elevAngleDegs = .50;
  LLH myCenter;

  if (ConfigRadarInfo::haveRadar(myRadarName)) {
    myCenter = ConfigRadarInfo::getLocation(myRadarName);
  } else {
    fLogSevere( "Couldn't find info for radar '{}', using DEM center which is probably not what you want.", myRadarName);
    myCenter = myDEM->getCenterLocation();
  }
  fLogInfo("Center location: {}", myCenter);

  Time myTime;
  LengthMs firstGateDistanceMeters = 0;
  LengthMs gateSpacing = 250;

  auto myRadialSet = RadialSet::Create("Percentage", "dbZ", myCenter, myTime,
      elevAngleDegs, firstGateDistanceMeters, gateSpacing, myNumRadials, myNumGates);

  myRadialSet->setRadarName(myRadarName);
  myRadialSet->setTime(Time::CurrentTime());

  // Fill me.  FIXME: Make API easier maybe
  auto dataPtr = myRadialSet->getFloat2D();

  dataPtr->fill(Constants::DataUnavailable);
  //  auto & data = myRadialSet->getFloat2DRef();
  auto & rs = *myRadialSet;

  // Create terrain blockage
  if (myDEM != nullptr) {
    myTerrainBlockage =
      std::make_shared<TerrainBlockageLak>(myDEM, rs.getLocation(), myRadarRangeKM, rs.getRadarName());
  }

  addRadials(*myRadialSet);

  // uh oh we're not an algorithm...so direct write only...
  // but we want the -o I think.  So we need to be able to generically add
  // the -o and -i etc. independently.  Oh yay refactors are fun
  // declareOutputOptions
  // declareInputOptions
  // writeOutputProduct(myRadialSet->getTypeName(), myRadialSet);
  IODataType::write(myRadialSet, "test.netcdf");

  // BeamBlockage test alpha
  rs.setTypeName("BeamBlockage");
  // rs.setDataAttributeValue(Constants::ColorMap, "Fuzzy");
  rs.setDataAttributeValue(Constants::ColorMap, "Percentage");
  rs.setDataAttributeValue(Constants::Units, "dimensionless");

  // RadialSet is indexed Azimuth, Gates...
  auto & data = rs.getFloat2DRef();

  for (size_t i = 0; i < myNumGates; ++i) {
    for (size_t j = 0; j < myNumRadials; ++j) {
      data[j][i] = 1 - (data[j][i] / myRadarValue);
    }
  }
  IODataType::write(myRadialSet, "test-beam.netcdf");

  terrainAngleChart(rs);

  // terrainAngleChart2(rs);
} // MakeFakeRadarData::execute

void
MakeFakeRadarData::terrainAngleChart(RadialSet& rs)
{
  // Create a DataGrid with terrain heights per unit
  // This is the superclass of RadialSet, LatLonGrid, etc.  But you can see
  // the power of this vs MRMS.  I should add more examples or video of this
  // as part of python series at some point
  // First chart hack gonna have to do a few passes and refactor stuff probably
  if (!myChart.empty()) {
    AngleDegs centerAzDegs = 315; // hit mountains KEWX hopefully or not...
    try{
      fLogInfo("Chart is '{}'", myChart);
      centerAzDegs = std::stoi(myChart);
    }catch (const std::exception& e)
    {
      fLogSevere("Couldn't parse azimuth angle from chart parameter, using {}", centerAzDegs);
    }
    const AngleDegs elevDegs = rs.getElevationDegs();

    // Create chart title as attribute for the python
    std::stringstream worker;
    worker << myRadarName << " az=" << centerAzDegs << ", elev=" << elevDegs << ", bw=" << myBeamWidthDegs;
    const std::string chartTitle = worker.str();

    fLogInfo("Create DataGrid for chart named: '{}'", chartTitle);

    fLogSevere("..... creating generic data grid class {}", myNumGates);
    Time aTime;
    LLH aLocation;
    auto myDataGrid = DataGrid::Create("TerrainElevationChartData", "dimensionless", aLocation, aTime, { myNumGates,
                                                                                                         100 },
        { "Range", "Height" });

    // Attributes can be a great way to pass strings and flags to your python:
    // Here I do the chart title
    myDataGrid->setString("ChartTitle", chartTitle);

    // This is the -unit -value ability from MRMS if you need units
    // myDataGrid->setDataAttributeValue("aAttribute", "AttributeName", "Dimensionless");

    myDataGrid->addFloat1D("BaseHeight", "Kilometers", { 0 }); // Y1
    auto & base = myDataGrid->getFloat1DRef("BaseHeight");

    myDataGrid->addFloat1D("TopHeight", "Kilometers", { 0 }); // Y2
    auto & top = myDataGrid->getFloat1DRef("TopHeight");

    myDataGrid->addFloat1D("BotHeight", "Kilometers", { 0 }); // Y3
    auto & bot = myDataGrid->getFloat1DRef("BotHeight");

    myDataGrid->addFloat1D("TerrainHeight", "Kilometers", { 0 }); // Y4
    auto & terrain = myDataGrid->getFloat1DRef("TerrainHeight");

    // Partial, per point blockage...
    myDataGrid->addFloat1D("TerrainPartial", "Dimensionless", { 0 }); // Y5
    auto & percent1 = myDataGrid->getFloat1DRef("TerrainPartial");

    // Full cumulative, per point blockage...
    myDataGrid->addFloat1D("TerrainFull", "Dimensionless", { 0 }); // Y5
    auto & percent2 = myDataGrid->getFloat1DRef("TerrainFull");

    // We'll use the gate range as the testing range
    myDataGrid->addFloat1D("Range", "Kilometers", { 0 }); // X
    auto & range = myDataGrid->getFloat1DRef("Range");

    // DEM projection.
    auto myDEMLookup = std::make_shared<LatLonGridProjection>(Constants::PrimaryDataName, myDEM.get());

    LLH theCenter = rs.getLocation();
    LengthKMs stationHeightKMs = theCenter.getHeightKM();

    LengthKMs startKM  = rs.getDistanceToFirstGateM() / 1000.0;
    LengthKMs rangeKMs = startKM;
    LengthKMs aTerrain;
    double gwKMs = myGateWidthM / 1000.0;
    for (int i = 0; i < myNumGates; ++i) {
      // Get height for gate center
      // Height of bottom of beam...
      AngleDegs outLatDegs, outLonDegs;
      AngleDegs topDegs    = elevDegs + 0.5 * myBeamWidthDegs;
      AngleDegs bottomDegs = elevDegs - 0.5 * myBeamWidthDegs;

      // Full calculate each time to any possible cumluative addition roundoff
      // or comment out to do addition
      rangeKMs = startKM + ((i * gwKMs) + (.5 * gwKMs)); // plus half gatewidth for center of gate

      // We get back pretty the exact same height doing this, which makes me think heights are correct
      // LengthKMs height = myTerrainBlockage->getHeightKM(topDegs, centerAzDegs, rangeKMs, outLatDegs, outLonDegs);
      LengthKMs height = Project::attenuationHeightKMs(stationHeightKMs, rangeKMs, topDegs);
      top[i] = height;

      height = Project::attenuationHeightKMs(stationHeightKMs, rangeKMs, elevDegs);
      // height  = myTerrainBlockage->getHeightKM(elevDegs, centerAzDegs, rangeKMs, outLatDegs, outLonDegs);
      base[i] = height;

      // I also want the lat/lon at the location for the stock terrain lookup.  Though I'm wondering now
      // if we make a polar terrain..which is kinda what Lak did, lol.  The issue is we kinda need terrain
      // values for a range over the gate width.  We could do terrain for each and then average, or
      // we can average the results.  The approach they used was average the terrain first, eh.  Less math
      // doing that, not sure it's accurate.  Interesting the centerAzDegs here jus gives us info to
      // get the lat/lon.  So we could make a function 'just' for that maybe
      // FIXME: BeamPath_AzRangeToLatLon should just take a height parameter
      height = myTerrainBlockage->getHeightKM(bottomDegs, centerAzDegs, rangeKMs, outLatDegs, outLonDegs);
      bot[i] = height;

      float fractionBlocked = 0;
      // My silly linear thing which isn't going to work with a 3D beam
      // float fractionBlocked = myTerrainBlockage->computePointPartialAt(myBeamWidthDegs,
      //    elevDegs,
      //    centerAzDegs,
      //    rangeKMs,
      //    aTerrain);

      double aHeightMeters = myDEMLookup->getValueAtLL(outLatDegs, outLonDegs);
      // Humm getting missing..we never check for it do we in the TerrainBlockage?
      if (aHeightMeters == Constants::MissingData) {
        terrain[i] = 0;
      } else {
        terrain[i] = aHeightMeters / 1000.0;
      }
      // Ok half power radius is based on beamwidth AND range.  Kill me, this took forever
      // to figure out.
      // Please make a difference for the love of all that is holy...
      // Battan (1973)


      // Ok let's try the formula from the 2003 Bech paper...

      // These do height at range independent of azimuth direction...
      LengthKMs c = Project::attenuationHeightKMs(stationHeightKMs, rangeKMs, elevDegs);
      LengthKMs d = Project::attenuationHeightKMs(stationHeightKMs, rangeKMs, bottomDegs);

      // But gonna try averaging the terrain across the beamwidth circle diameter.  I think we
      // could use an artificial radialset and sample in 'rings' around the gates.  We really want
      // the full ray-traced terrain filling the circle.  This could be done on a GPU with shaders
      // fairly quickly which could be interesting.
      // So in theory, if we just averaged y from Bech then this would be the dy they talk about...
      // so we just need to get terrain in a sample range and do a y average.
      // Why not we can try anything at this point?

      // LengthKMs a = c-d;   // a    Not 100% sure.  Seems right, eh what the heck.  Should be circle radius
      //                              NOOOOOOOOOOOOOOOOoooooooooooooooooooo

      // LengthKMs a = (rangeKMs *  myBeamWidthDegs*DEG_TO_RAD)/2.0;  // Kill me..  KILL ME
      LengthKMs a = (rangeKMs * myBeamWidthDegs * DEG_TO_RAD) / 2.0; // Kill me..  KILL ME
      //      if (a < 0){ a = -a;} /// "shouldn't" hit anything

      // Ok y is the value from circle center up or down to terrain.  This we want to supersample over
      // the circle.  So if we go from -.5 beamwidth left to +.5 beamwidth right in azimuth, should work...
      // Lak is doing something similar with the rays.  I just want to see how much the results are
      // affected by this
      size_t numSamples = 500;

      LengthKMs TerrainAverageKMs = 0;
      AngleDegs left     = centerAzDegs - .5 * myBeamWidthDegs;
      AngleDegs right    = centerAzDegs + 5 * myBeamWidthDegs;
      AngleDegs deltaDeg = (right - left) / numSamples;

      // Scan dy over the diameter of Bech's beam circle
      // This is my complete non-meteorological idea..to terrain sample over the entire gate area
      // to get the terrain value used.
      LengthKMs TerrainMax    = -9999;
      LengthKMs TerrainCenter = -9999;
      for (size_t k = 0; k < numSamples; k++) {
        AngleDegs terrainElevDegs = 0; // elevDegs or project to ground...wow no difference still?
        // start of gate range sample
        LengthKMs h1 = myTerrainBlockage->getHeightKM(terrainElevDegs, left, rangeKMs - (.5 * gwKMs), outLatDegs,
            outLonDegs);
        LengthKMs t1 = myDEMLookup->getValueAtLL(outLatDegs, outLonDegs); // not just yet
        if (t1 == Constants::MissingData) {                               // FIXME: API this should be internally done
          t1 = 0;
        } else {
          t1 = t1 / 1000.0;
        }

        // middle of gate range sample
        LengthKMs h2 = myTerrainBlockage->getHeightKM(terrainElevDegs, left, rangeKMs, outLatDegs, outLonDegs);
        LengthKMs t2 = myDEMLookup->getValueAtLL(outLatDegs, outLonDegs); // not just yet
        if (t2 == Constants::MissingData) {
          t2 = 0;
        } else {
          t2 = t2 / 1000.0;
        }
        if (k == 0) {
          TerrainCenter = t2;
        }

        // end of gate range terrain sample
        LengthKMs h3 = myTerrainBlockage->getHeightKM(terrainElevDegs, left, rangeKMs + (.5 * gwKMs), outLatDegs,
            outLonDegs);
        LengthKMs t3 = myDEMLookup->getValueAtLL(outLatDegs, outLonDegs); // not just yet
        if (t3 == Constants::MissingData) {                               // FIXME: API this should be internally done
          t3 = 0;
        } else {
          t3 = t3 / 1000.0;
        }

        if (t1 > TerrainMax) { TerrainMax = t1; }
        if (t2 > TerrainMax) { TerrainMax = t2; }
        if (t3 > TerrainMax) { TerrainMax = t3; }

        TerrainAverageKMs = TerrainAverageKMs + t1 + t2 + t3;

        // fLogSevere(" totalY, y current, degs , terrain {}, {}, {}, {}", 
        //   totalY, yi, left);
        left += deltaDeg;
      }
      TerrainAverageKMs /= (numSamples * 3);

      LengthKMs y = TerrainCenter - c;
      // LengthKMs y = TerrainAverageKMs-c;
      // LengthKMs y = TerrainMax-c;
      // LengthKMs y = terrain[i]-c;

      // Direct from the Bech Et Al 2003 paper, though it doesn't seem to change the results much,
      // if anything it 'dampens' the terrain effect slightly
      if (y >= a) {
        fractionBlocked = 1.0;
      } else if (y <= -a) {
        fractionBlocked = 0.0;
      } else { // Partial Beam Blockage PBB calculation
        double a2  = a * a;
        double y2  = y * y;
        double num = (y * sqrt(a2 - y2)) + (a2 * asin(y / a)) + (M_PI * a2 / 2.0); // Part of circle
        double dem = M_PI * a2;                                                    // Full circle

        // Ok metpy does it different ?  really? Is this the same output
        // This is just an optimization I think canceling a
        //      double ya = y/a;
        //      double num = (ya*sqrt(a2 - y2))+(a*asin(ya))+(M_PI * a/2.0); // Part of circle
        //      double dem = M_PI*a; // Full circle
        fractionBlocked = num / dem; // % covered
      }

      percent1[i] = fractionBlocked;

      #if 0
      test passes
      float diff = terrain[i] - aTerrain;
      if (diff < 0) { diff = -diff; }

      if (terrain[i] != aTerrain) { // KM !=  KM
        // fLogSevere("TERRAIN? {} != {}", terrain[i], aTerrain);
        fLogSevere("TERRAIN? {}",diff);
      }
      #endif
      range[i] = rangeKMs;

      // Next x on graph...
      rangeKMs += (myGateWidthM / 1000.0);
    }

    // Cummulate the terrain blockage percentages
    float greatest = -1000;
    for (int i = 0; i < myNumGates; ++i) {
      float v = percent1[i];
      if (v > greatest) {
        greatest = v;
      } else {
        v = greatest;
      }
      percent2[i] = v;
    }

    IODataType::write(myDataGrid, "test-grid.netcdf");
  }
  fLogSevere("EXECUTED 4  Max terrain in gate.");
} // MakeFakeRadarData::terrainAngleChart

void
MakeFakeRadarData::terrainAngleChart2(RadialSet& rs)
{
  // Keeping this for moment..but also have copy in TerrainBlockage,
  // more work needed...
  const size_t numX = myNumRadials;
  const size_t numY = myNumGates;

  // We 'could' make a RadialSet here I think, or not
  Time aTime;
  LLH aLocation;
  auto myDataGrid = DataGrid::Create("NONE", "dimensionless", aLocation, aTime, { numX, numY }, { "Azimuth", "Gate" });

  // DEM projection. FIXME: make global probably better
  auto myDEMLookup = std::make_shared<LatLonGridProjection>(Constants::PrimaryDataName, myDEM.get());

  std::string chartTitle = "Terrain";

  myDataGrid->setString("ChartTitle", chartTitle);

  myDataGrid->addFloat2D("TerrainHeight", "Meters", { 0, 1 }); // Y1  API just addFloat, use the dimensions
  auto & base = myDataGrid->getFloat2DRef("TerrainHeight");

  // Kinda stock for marching over radial set, should be a visitor probably
  LengthKMs startKM = rs.getDistanceToFirstGateM() / 1000.0;
  LengthKMs rangeKMs = startKM;
  AngleDegs outLatDegs, outLonDegs;
  LengthKMs gwKMs           = myGateWidthM / 1000.0;
  AngleDegs terrainElevDegs = 0; // elevDegs or project to ground
  AngleDegs azDegs          = 0;

  for (int r = 0; r < numX; ++r) {
    for (int g = 0; g < numY; ++g) {
      rangeKMs = startKM + ((g * gwKMs) + (.5 * gwKMs)); // plus half gatewidth for center of gate
      // start of gate range sample
      LengthKMs h1 =
        myTerrainBlockage->getHeightKM(terrainElevDegs, azDegs + (.5 * myAzimuthalDegs), rangeKMs - (.5 * gwKMs),
          outLatDegs, outLonDegs);
      LengthKMs t1 = myDEMLookup->getValueAtLL(outLatDegs, outLonDegs); // not just yet
      if (t1 == Constants::MissingData) {                               // FIXME: API this should be internally done
        t1 = 0;
      } else {
        t1 = t1 / 1000.0;
      }
      if (t1 < 0) { t1 = 0; } // temp pin kilometers for graphing
      if (t1 > 2) { t1 = 2; }
      base[r][g] = t1;
      // base[r][g] = g; // r max 360, g max 20  azimuth, gate.
      // base[x][g] ==> going down variable...also goes it in matlib.  So it's really y.
    }
    azDegs += myAzimuthalDegs;
  }

  IODataType::write(myDataGrid, "angle-chart-data.netcdf");
} // MakeFakeRadarData::terrainAngleChart2

void
MakeFakeRadarData::addRadials(RadialSet& rs)
{
  AngleDegs bwDegs = myBeamWidthDegs;
  static Time t    = Time::CurrentTime() - TimeDuration::Hours(1000);

  t = t + TimeDuration::Minutes(1);
  LengthKMs gwKMs = myGateWidthM / 1000.0; // FIXME: Make a meters
  const AngleDegs elevDegs = rs.getElevationDegs();

  // Fill beamwidth
  auto bwPtr = rs.getFloat1D(RadialSet::BeamWidth);

  bwPtr->fill(myBeamWidthDegs);

  // Fill gatewidth
  auto gwPtr = rs.getFloat1D(RadialSet::GateWidth);

  gwPtr->fill(myGateWidthM);

  AngleDegs azDegs = 0;
  auto & azData    = rs.getFloat1DRef(RadialSet::Azimuth);

  auto & data = rs.getFloat2DRef();

  LengthKMs aTerrain;

  for (int j = 0; j < myNumRadials; ++j) {
    azData[j] = azDegs; // Set azimuth degrees

    const AngleDegs centerAzDegs = azDegs + (0.5 * myAzimuthalDegs);

    for (int i = 0; i < myNumGates; ++i) {
      float gateValue = 60.0 * i / myNumGates; // without terrain
      if (myTerrainBlockage != nullptr) {
        //  float fractionBlocked = myTerrainBlockage->computePointPartialAt(myBeamWidthDegs,
        //      elevDegs,
        //      centerAzDegs,
        //      (i * gwKMs) + (.5 * gwKMs), aTerrain); // plus half gatewidth for center

        /*
         *      float fractionBlocked = myTerrainBlockage->computeFractionBlocked(myBeamWidthDegs,
         *          elevDegs,
         *          centerAzDegs,
         *          i * gwKMs);
         */
        // gateValue = myRadarValue * (1 - fractionBlocked);
        //  gateValue = fractionBlocked;
        gateValue = aTerrain * 1000.0; // terrain in meters
      }
      // Ok WHAT to store in the netcdf...?
      data[j][i] = gateValue; // data value
    }
    azDegs += myAzimuthalDegs;
  }

  // Cumulative in polar is sooo much easier than grid
  // Silly simple make it the greatest along the radial path
  for (int j = 0; j < myNumRadials; ++j) {
    float greatest = -1000; // percentage
    for (int i = 0; i < myNumGates; ++i) {
      float& v = data[j][i];

      // Partial
      // v = myRadarValue * (1 - v);
      // continue;

      // Cumulative
      if (v > greatest) {
        greatest = v;
      } else {
        v = greatest;
      }

      /*
       *  Leave as cummulative or set to converted value, right?
       *    // Change final value from percent to data value
       *    if (greatest > .50) {
       *      v = Constants::DataUnavailable;
       *    } else {
       *      v = myRadarValue * (1 - greatest);
       *    }
       */
    }
  }
} // MakeFakeRadarData::addRadials

// FIXME: generic routing in rTerrainBlockage
void
MakeFakeRadarData::readTerrain()
{
  if (myTerrainPath.empty()) {
    fLogInfo("No terrain path provided, running without terrain information");
  } else if (myDEM == nullptr) {
    {
      auto d = IODataType::read<LatLonGrid>(myTerrainPath);
      if (d != nullptr) {
        fLogInfo("Terrain DEM read: ", myTerrainPath);
        myDEM = d;
      } else {
        fLogSevere("Failed to read in Terrain DEM LatLonGrid at {}", myTerrainPath);
        exit(1);
      }
    }
  }
}

int
main(int argc, char * argv[])
{
  MakeFakeRadarData alg = MakeFakeRadarData();

  alg.executeFromArgs(argc, argv);
}
