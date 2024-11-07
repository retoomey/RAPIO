#include "rRadialSet.h"
#include "rRadialSetProjection.h"

#include "rError.h"
#include "rUnit.h"
#include "rLLH.h"

#include "rProcessTimer.h"

using namespace rapio;
using namespace std;

std::string
RadialSet::getGeneratedSubtype() const
{
  return (formatString(getElevationDegs(), 5, 2)); // RadialSet
}

RadialSet::RadialSet() : myElevAngleDegs(0), myElevCos(1), myElevTan(0), myFirstGateDistanceM(0),
  myHaveTerrain(false)
{
  setDataType("RadialSet");
}

std::shared_ptr<RadialSet>
RadialSet::Create(
  const std::string& TypeName,
  const std::string& Units,
  const LLH        & center,
  const Time       & datatime,
  const float      elevationDegrees,
  const float      firstGateDistanceMeters,
  const float      gateWidthMeters,
  const size_t     num_radials,
  const size_t     num_gates)
{
  auto newonesp = std::make_shared<RadialSet>();

  newonesp->init(TypeName, Units, center, datatime, elevationDegrees, firstGateDistanceMeters, gateWidthMeters,
    num_radials, num_gates);
  return newonesp;
}

std::shared_ptr<RadialSet>
RadialSet::Clone()
{
  auto nsp = std::make_shared<RadialSet>();

  RadialSet::deep_copy(nsp);
  return nsp;
}

void
RadialSet::deep_copy(std::shared_ptr<RadialSet> nsp)
{
  DataGrid::deep_copy(nsp);

  // Copy our stuff
  auto & n = *nsp;

  n.myElevAngleDegs      = myElevAngleDegs; // in attributes also
  n.myElevCos            = myElevCos;
  n.myElevTan            = myElevTan;
  n.myFirstGateDistanceM = myFirstGateDistanceM; // in attributes also
  n.myHaveTerrain        = myHaveTerrain;
}

bool
RadialSet::init(
  const std::string& TypeName,
  const std::string& Units,
  const LLH        & center,
  const Time       & datatime,
  const float      elevationDegrees,
  const float      firstGateDistanceMeters,
  const float      gateWidthMeters,
  const size_t     num_radials,
  const size_t     num_gates)
{
  DataGrid::init(TypeName, Units, center, datatime, { num_radials, num_gates }, { "Azimuth", "Gate" });

  setElevationDegs(elevationDegrees);
  myFirstGateDistanceM = firstGateDistanceMeters;

  setDataType("RadialSet");
  /** Primary data storage */
  addFloat2D(Constants::PrimaryDataName, Units, { 0, 1 });

  /** Azimuth per radial */
  auto a = addFloat1D("Azimuth", "Degrees", { 0 });

  // For a default azimuth, let's divide a circle by the number of radials
  // and calculate beamwidth based on that for each radial.  Obvious this is
  // just a simple default.  Anyone doing advanced stuff would probably fill
  // in this data themselves.
  auto& azimuths  = a->ref();
  float increment = 360.0 / num_radials;
  float at        = 0.0;

  for (size_t z = 0; z < num_radials; ++z) {
    azimuths[z] = at;
    at += increment;
  }

  /** Beamwidth per radial */
  addFloat1D("BeamWidth", "Degrees", { 0 }, increment);

  /** Gate width per radial. */
  addFloat1D("GateWidth", "Meters", { 0 }, gateWidthMeters);
  return true;
} // RadialSet::init

bool
RadialSet::initFromGlobalAttributes()
{
  bool success = true;

  DataType::initFromGlobalAttributes();

  // TypeName check, such as Reflectivity or Velocity
  if (myTypeName == "not set") {
    LogSevere("Missing TypeName attribute such as Reflectivity.\n");
    success = false;
  }

  // -------------------------------------------------------
  // Elevation
  if (!getDouble("Elevation", myElevAngleDegs)) {
    LogSevere("Missing Elevation in degrees attribute\n");
    success = false;
  }

  // -------------------------------------------------------
  // Range to first gate
  if (!getDouble("RangeToFirstGate", myFirstGateDistanceM)) {
    LogInfo("Missing RangeToFirstGate attribute, will be zero.\n");
    myFirstGateDistanceM = 0;
  }

  return success;
} // RadialSet::initFromGlobalAttributes

void
RadialSet::updateGlobalAttributes(const std::string& encoded_type)
{
  // Note: Datatype updates the attributes -unit -value specials,
  // so don't add any after this
  DataType::updateGlobalAttributes(encoded_type);

  // Radial set only global attributes
  const double elevDegrees = getElevationDegs();

  setDouble("Elevation", elevDegrees);
  setString("ElevationUnits", "Degrees");

  const double firstGateM = getDistanceToFirstGateM();

  setDouble("RangeToFirstGate", firstGateM);
  setString("RangeToFirstGateUnits", "Meters");
}

std::shared_ptr<DataProjection>
RadialSet::getProjection(const std::string& layer)
{
  if (myDataProjection == nullptr) {
    myDataProjection = std::make_shared<RadialSetProjection>(layer, this);
  }
  return myDataProjection;
}

void
RadialSet::initTerrain()
{
  if (!myHaveTerrain) {
    if (!getFloat2D(Constants::TerrainCBBPercent)) { // Maybe disk loaded
      addFloat2D(Constants::TerrainCBBPercent, "Dimensionless", { 0, 1 });
    }
    if (!getFloat2D(Constants::TerrainPBBPercent)) {
      addFloat2D(Constants::TerrainPBBPercent, "Dimensionless", { 0, 1 });
    }
    if (!getByte2D(Constants::TerrainBeamBottomHit)) {
      addByte2D(Constants::TerrainBeamBottomHit, "Dimensionless", { 0, 1 });
    }
  }
  myHaveTerrain = true;
}

void
RadialSet::postRead(std::map<std::string, std::string>& keys)
{
  // FIXME: Make it stay compressed for things like rcopy, probably
  // using a key
  unsparse2D(myDims[0].size(), myDims[1].size(), keys);
} // RadialSet::postRead

void
RadialSet::preWrite(std::map<std::string, std::string>& keys)
{
  sparse2D(); // Standard sparse of primary data (add dimension)
}

void
RadialSet::postWrite(std::map<std::string, std::string>& keys)
{
  unsparseRestore();
}
