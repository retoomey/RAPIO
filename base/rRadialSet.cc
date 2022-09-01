#include "rRadialSet.h"

#include "rError.h"
#include "rUnit.h"
#include "rLLH.h"

// #include <cassert>

using namespace rapio;
using namespace std;

std::string
RadialSet::getGeneratedSubtype() const
{
  return (formatString(getElevationDegs(), 5, 2)); // RadialSet
}

RadialSet::RadialSet() : myElevAngleDegs(0), myElevCos(1), myElevTan(0), myFirstGateDistanceM(0), myLookup(nullptr)
{
  myDataType = "RadialSet";
}

std::shared_ptr<RadialSet>
RadialSet::Create(
  const std::string& TypeName,
  const std::string& Units,
  const LLH        & center,
  const Time       & datatime,
  const float      elevationDegrees,
  const float      firstGateDistanceMeters,
  const size_t     num_radials,
  const size_t     num_gates)
{
  auto r = std::make_shared<RadialSet>();

  if (r == nullptr) {
    LogSevere("Couldn't create RadialSet.\n");
  } else {
    // We post constructor fill in details because many of the factories like netcdf 'chain' layers and settings
    r->init(TypeName, Units, center, datatime, elevationDegrees, firstGateDistanceMeters, num_radials, num_gates);
  }

  return r;
}

void
RadialSet::init(
  const std::string& TypeName,
  const std::string& Units,
  const LLH        & center,
  const Time       & time,
  const float      elevationDegrees,
  const float      firstGateDistanceMeters,
  const size_t     num_radials,
  const size_t     num_gates)
{
  /** Declare/update the dimensions */
  setTypeName(TypeName);
  setDataAttributeValue("Unit", "dimensionless", Units);
  myLocation      = center;
  myTime          = time;
  myElevAngleDegs = elevationDegrees;
  myElevCos       = cos(myElevAngleDegs * DEG_TO_RAD);
  myElevTan       = tan(myElevAngleDegs * DEG_TO_RAD);

  myFirstGateDistanceM = firstGateDistanceMeters;

  declareDims({ num_radials, num_gates }, { "Azimuth", "Gate" });
  addFloat2D(Constants::PrimaryDataName, Units, { 0, 1 });

  // These are the only ones we force...

  /** Azimuth per radial */
  // addFloat1D("Azimuth", "Degrees", {0}, 0.0f);
  addFloat1D("Azimuth", "Degrees", { 0 });

  /** Beamwidth per radial */
  // addFloat1D("BeamWidth", "Degrees", {0}, 1.0f);
  addFloat1D("BeamWidth", "Degrees", { 0 });

  /** Gate width per radial */
  // addFloat1D("GateWidth", "Meters", {0}, 1000.0f);
  addFloat1D("GateWidth", "Meters", { 0 });
}

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
  return std::make_shared<RadialSetProjection>(layer, this);
}
