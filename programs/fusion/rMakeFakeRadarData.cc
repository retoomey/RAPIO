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
  AngleDegs elevAngleDegs = .50;
  LLH myCenter = myDEM->getCenterLocation(); // cheat for moment.  We need radar info I guess, bleh
  Time myTime;
  LengthMs firstGateDistanceMeters = 0;

  auto myRadialSet = RadialSet::Create("Reflectivity", "dbZ", myCenter, myTime,
      elevAngleDegs, firstGateDistanceMeters, myNumRadials, myNumGates);

  myRadialSet->setRadarName("KTLX");

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
} // MakeFakeRadarData::execute

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
        float fractionBlocked = myTerrainBlockage->computeFractionBlocked(myBeamWidthDegs,
            elevDegs,
            centerAzDegs,
            i * myGateWidthM);
        gateValue = myRadarValue * (1 - fractionBlocked);
      }
      data[j][i] = gateValue;
    }
    azDegs += myAzimuthalDegs;
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
