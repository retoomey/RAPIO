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
  return (formatString(getElevation(), 5, 2)); // RadialSet
}

RadialSet::RadialSet() : myElevAngleDegs(0), myFirstGateDistanceM(0), myLookup(nullptr)
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
  const size_t     num_gates,
  const float      value)
{
  auto r = std::make_shared<RadialSet>();

  if (r == nullptr) {
    LogSevere("Couldn't create RadialSet.\n");
  } else {
    // We post constructor fill in details because many of the factories like netcdf 'chain' layers and settings
    r->init(TypeName, Units, center, datatime, elevationDegrees, firstGateDistanceMeters, num_radials, num_gates,
      value);

    // FIXME: Do filling of default value here...

    /*
     * auto array = radialSet.getFloat2D("primary");
     * auto& data = array->ref();
     * for (size_t i = 0; i < num_radials; ++i) {
     *  for (size_t j = 0; j < num_gates; ++j) {
     *    azimuths[i]   = start_az;
     *    beamwidths[i] = beam_width;
     *    gatewidths[i] = gate_width;
     *    data[i][j] = value;
     *  }
     * }
     * auto azimuthsA   = radialSet.getFloat1D("Azimuth");
     * auto& azimuths   = azimuthsA->ref();
     * auto beamwidthsA = radialSet.getFloat1D("BeamWidth");
     * auto& beamwidths = beamwidthsA->ref();
     * auto gatewidthsA = radialSet.getFloat1D("GateWidth");
     * auto& gatewidths = gatewidthsA->ref();
     *
     */
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
  const size_t     num_gates,
  const float      fill)
{
  /** Declare/update the dimensions */
  setTypeName(TypeName);
  setDataAttributeValue("Unit", "dimensionless", Units);
  myLocation           = center;
  myTime               = time;
  myElevAngleDegs      = elevationDegrees;
  myFirstGateDistanceM = firstGateDistanceMeters;

  declareDims({ num_radials, num_gates }, { "Azimuth", "Gate" });
  addFloat2D("primary", Units, { 0, 1 });

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

  // Shared coded with LatLonGrid.  Gotta be careful moving 'up'
  // since the DataGrid can read general netcdf which could be
  // missing this information.  Still thinking about it.

  // TypeName check, such as Reflectivity or Velocity
  if (!getString(Constants::TypeName, myTypeName)) {
    LogSevere("Missing TypeName attribute such as Reflectivity.\n");
    success = false;
  }

  // -------------------------------------------------------
  // Location
  float latDegs  = 0;
  float lonDegs  = 0;
  float htMeters = 0;
  success &= getFloat(Constants::Latitude, latDegs);
  success &= getFloat(Constants::Longitude, lonDegs);
  success &= getFloat(Constants::Height, htMeters);
  if (success) {
    myLocation = LLH(latDegs, lonDegs, htMeters / 1000.0);
  } else {
    LogSevere("Missing Location attribute\n");
  }

  // -------------------------------------------------------
  // Time
  long timesecs = 0;
  if (getLong(Constants::Time, timesecs)) {
    // optional
    double f = 0.0;
    getDouble(Constants::FractionalTime, f);
    // Cast long to time_t..hummm
    myTime = Time(timesecs, f);
  } else {
    LogSevere("Missing Time attribute\n");
    success = false;
  }

  // Radial set only

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
  const double elevDegrees = getElevation();
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
