#include "rMakeFakeRadarData.h"

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
    LogSevere("Ok you need to actually have terrain for this first alpha\n");
    exit(1);
  }
}

void
MakeFakeRadarData::execute()
{
  LogInfo("Radarname: " << myRadarName << "\n");
  LogInfo("BeamWidth: " << myBeamWidthDegs << "\n");
  LogInfo("GateWidth: " << myGateWidthM << "\n");
  LogInfo("Radar range: " << myRadarRangeKM << "\n");
  LogInfo("Azimuthal spacing: " << myAzimuthalDegs << "\n");

  // We don't have vcp so what to do.  FIXME: probably new parameters right?
  // FIXME: have vcp lists?  Bleh we got into trouble because of that in first place
  // AngleDegs elevAngleDegs = .50;
  AngleDegs elevAngleDegs = .50;
  LLH myCenter;

  if (ConfigRadarInfo::haveRadar(myRadarName)) {
    myCenter = ConfigRadarInfo::getLocation(myRadarName);
  } else {
    LogSevere(
      "Couldn't find info for radar '" << myRadarName << "', using DEM center which is probably not what you want.\n");
    myCenter = myDEM->getCenterLocation();
  }
  LogInfo("Center location: " << myCenter << "\n");

  Time myTime;
  LengthMs firstGateDistanceMeters = 0;

  auto myRadialSet = RadialSet::Create("Reflectivity", "dbZ", myCenter, myTime,
      elevAngleDegs, firstGateDistanceMeters, myNumRadials, myNumGates);

  myRadialSet->setRadarName("KTLX");
  myRadialSet->setTime(Time::CurrentTime());

  // Fill me.  FIXME: Make API easier maybe
  auto dataPtr = myRadialSet->getFloat2D();

  dataPtr->fill(Constants::DataUnavailable);
  auto & data = myRadialSet->getFloat2D()->ref();
  auto & rs   = *myRadialSet;

  // Create terrain blockage
  if (myDEM != nullptr) {
    myTerrainBlockage =
      std::make_shared<TerrainBlockage>(myDEM, rs.getLocation(), myRadarRangeKM, rs.getRadarName());
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
  rs.setDataAttributeValue(Constants::ColorMap, "Fuzzy");
  rs.setDataAttributeValue(Constants::Unit, "dimensionless");
  //  auto & data = rs.getFloat2D()->ref();
  for (size_t i = 0; i < myNumGates; ++i) {
    for (size_t j = 0; j < myNumRadials; ++j) {
      data[i][j] = 1 - (data[i][j] / myRadarValue); // humm
    }
  }
  IODataType::write(myRadialSet, "test-beam.netcdf");

  terrainAngleChart(rs);
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
      LogInfo("Chart is '" << myChart << "'\n");
      centerAzDegs = std::stoi(myChart);
    }catch (const std::exception& e)
    {
      LogSevere("Couldn't parse azimuth angle from chart parameter, using " << centerAzDegs << "\n");
    }
    const AngleDegs elevDegs = rs.getElevationDegs();

    // Create chart title as attribute for the python
    std::stringstream worker;
    worker << myRadarName << " az=" << centerAzDegs << ", elev=" << elevDegs << ", bw=" << myBeamWidthDegs;
    const std::string chartTitle = worker.str();

    LogInfo("Create DataGrid for chart named: '" << chartTitle << "'\n");

    LogSevere("..... creating generic data grid class " << myNumGates << "\n");
    auto myDataGrid = DataGrid::Create({ myNumGates, 100 }, { "Range", "Height" });

    // Attributes can be a great way to pass strings and flags to your python:
    // Here I do the chart title
    myDataGrid->setString("ChartTitle", chartTitle);

    // This is the -unit -value ability from MRMS if you need units
    // myDataGrid->setDataAttributeValue("aAttribute", "AttributeName", "Dimensionless");

    myDataGrid->setTypeName("TerrainElevationChartData");

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

    // auto& gw  = rs.getGateWidthVector()->ref();
    LengthKMs startKM  = rs.getDistanceToFirstGateM() / 1000.0;
    LengthKMs rangeKMs = startKM;
    double gwKMs       = myGateWidthM / 1000.0;
    for (int i = 0; i < myNumGates; ++i) {
      // Get height for gate center
      // Height of bottom of beam...
      AngleDegs outLatDegs, outLonDegs;
      AngleDegs topDegs    = elevDegs + 0.5 * myBeamWidthDegs;
      AngleDegs bottomDegs = elevDegs - 0.5 * myBeamWidthDegs;

      float fractionBlocked = myTerrainBlockage->computePointPartialAt(myBeamWidthDegs,
          elevDegs,
          centerAzDegs,
          (i * gwKMs) + (.5 * gwKMs)); // plus half gatewidth for center
      percent1[i] = fractionBlocked;

      LengthKMs height = myTerrainBlockage->getHeightKM(topDegs, centerAzDegs, rangeKMs, outLatDegs, outLonDegs);
      top[i] = height;

      height  = myTerrainBlockage->getHeightKM(elevDegs, centerAzDegs, rangeKMs, outLatDegs, outLonDegs);
      base[i] = height;

      height = myTerrainBlockage->getHeightKM(bottomDegs, centerAzDegs, rangeKMs, outLatDegs, outLonDegs);
      bot[i] = height;

      double aHeightMeters = myDEMLookup->getValueAtLL(outLatDegs, outLonDegs);
      // Humm getting missing..we never check for it do we?
      if (aHeightMeters == Constants::MissingData) {
        terrain[i] = 0;
      } else {
        terrain[i] = aHeightMeters / 1000.0;
      }

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
} // MakeFakeRadarData::terrainAngleChart

void
MakeFakeRadarData::addRadials(RadialSet& rs)
{
  AngleDegs bwDegs = myBeamWidthDegs;
  static Time t    = Time::CurrentTime() - TimeDuration::Hours(1000);

  t = t + TimeDuration::Minutes(1);
  LengthKMs gwKMs = myGateWidthM / 1000.0; // FIXME: Make a meters
  const AngleDegs elevDegs = rs.getElevationDegs();

  // Fill beamwidth
  auto bwPtr = rs.getFloat1D("BeamWidth");

  bwPtr->fill(myBeamWidthDegs);

  // Fill gatewidth
  auto gwPtr = rs.getFloat1D("GateWidth");

  gwPtr->fill(myGateWidthM);

  AngleDegs azDegs = 0;
  auto & azData    = rs.getFloat1D("Azimuth")->ref();

  auto & data = rs.getFloat2D()->ref();

  for (int j = 0; j < myNumRadials; ++j) {
    azData[j] = azDegs; // Set azimuth degrees

    const AngleDegs centerAzDegs = azDegs + (0.5 * myAzimuthalDegs);

    for (int i = 0; i < myNumGates; ++i) {
      float gateValue = 60.0 * i / myNumGates; // without terrain
      if (myTerrainBlockage != nullptr) {
        float fractionBlocked = myTerrainBlockage->computePointPartialAt(myBeamWidthDegs,
            elevDegs,
            centerAzDegs,
            (i * gwKMs) + (.5 * gwKMs)); // plus half gatewidth for center

        /*
         *      float fractionBlocked = myTerrainBlockage->computeFractionBlocked(myBeamWidthDegs,
         *          elevDegs,
         *          centerAzDegs,
         *          i * gwKMs);
         */
        // gateValue = myRadarValue * (1 - fractionBlocked);
        gateValue = fractionBlocked;
      }
      data[j][i] = gateValue;
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
      v = myRadarValue * (1 - v);
      continue;

      // Cumulative
      if (v > greatest) {
        greatest = v;
      } else {
        v = greatest;
      }
      // Change final value from percent to data value
      if (greatest > .50) {
        v = Constants::DataUnavailable;
      } else {
        v = myRadarValue * (1 - greatest);
      }
    }
  }
} // MakeFakeRadarData::addRadials

// FIXME: generic routing in rTerrainBlockage
void
MakeFakeRadarData::readTerrain()
{
  if (myTerrainPath.empty()) {
    LogInfo("No terrain path provided, running without terrain information\n");
  } else if (myDEM == nullptr) {
    {
      auto d = IODataType::read<LatLonGrid>(myTerrainPath);
      if (d != nullptr) {
        LogInfo("Terrain DEM read: " << myTerrainPath << "\n");
        myDEM = d;
      } else {
        LogSevere("Failed to read in Terrain DEM LatLonGrid at " << myTerrainPath << "\n");
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
