#include "rRadialSet.h"

#include "rError.h"
#include "rUnit.h"
#include "rIJK.h"
#include "rLLH.h"

#include <cassert>

using namespace rapio;
using namespace std;

RadialSet::RadialSet()
{
  myDataType = "RadialSet";
  init(0, 0);
}

RadialSet::RadialSet(const LLH& location,
  const Time                  & time)
{
  // Lookup for read/write factories
  myDataType = "RadialSet";
  myLocation = location;
  myTime     = time;
  myFirstGateDistanceM = 0;
  init(0, 0);
}

std::string
RadialSet::getGeneratedSubtype() const
{
  return (formatString(getElevation(), 5, 2)); // RadialSet
}

double
RadialSet::getElevation() const
{
  return (myElevAngleDegs);
}

void
RadialSet::init(size_t num_radials, size_t num_gates, const float fill)
{
  /** Declare/update the dimensions */
  declareDims({ num_radials, num_gates }, { "Azimuth", "Gate" });

  // Fill values are meaningless since we start off 0,0 for now...
  // FIXME: On a resize maybe fill in the default arrays?

  /** As a grid of data */
  // addFloat2D("primary", "Dimensionless", {0,1}, fill);
  addFloat2D("primary", "Dimensionless", { 0, 1 });

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

/*
 * void
 * RadialSet::resize(size_t num_radials, size_t num_gates, const float fill)
 * {
 * declareDims({num_radials, num_gates});
 * }
 */

size_t
RadialSet::getNumGates()
{
  return myDims[1].size();
}

size_t
RadialSet::getNumRadials()
{
  return myDims[0].size();
}

void
RadialSet::setElevation(const double& targetElev)
{
  myElevAngleDegs = targetElev;
}

bool
RadialSet::initFromGlobalAttributes()
{
  bool success = true;

  // Shared coded with LatLonGrid.  Gotta be careful moving 'up'
  // since the DataGrid can read general netcdf which could be
  // missing this information.  Still thinking about it.

  // TypeName check, such as Reflectivity or Velocity
  auto aTypeName = myAttributes->get<std::string>(Constants::TypeName);

  if (!aTypeName) {
    LogSevere("Missing TypeName attribute such as Reflectivity.\n");
    success = false;
  } else {
    myTypeName = *aTypeName;
  }

  // -------------------------------------------------------
  // Location
  auto lat = myAttributes->get<double>(Constants::Latitude);
  if (!lat) { success = false; }
  auto lon = myAttributes->get<double>(Constants::Longitude);
  if (!lon) { success = false; }
  auto ht = myAttributes->get<double>(Constants::Height);
  if (!ht) { success = false; }
  if (success) {
    myLocation = LLH(*lat, *lon, *ht / 1000.0);
  } else {
    LogSevere("Missing Location attribute\n");
  }

  // -------------------------------------------------------
  // Time
  auto timesecs = myAttributes->get<long>(Constants::Time);
  if (timesecs) {
    double f        = 0.0;
    auto fractional = myAttributes->get<double>(Constants::FractionalTime);
    if (fractional) {
      f = *fractional;
    }
    // Cast long to time_t..hummm
    myTime = Time(*timesecs, f);
  } else {
    LogSevere("Missing Time attribute\n");
    success = false;
  }

  // Radial set only

  // -------------------------------------------------------
  // Elevation
  auto elev = myAttributes->get<double>("Elevation");
  if (!elev) {
    LogSevere("Missing Elevation in degrees attribute\n");
    success = false;
  } else {
    myElevAngleDegs = *elev;
  }

  // -------------------------------------------------------
  // Range to first gate
  auto range = myAttributes->get<double>("RangeToFirstGate");
  if (range) {
    myFirstGateDistanceM = *range;
  } else {
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
  myAttributes->put<double>("Elevation", elevDegrees);
  myAttributes->put<std::string>("ElevationUnits", "Degrees");

  const double firstGateM = getDistanceToFirstGateM();
  myAttributes->put<double>("RangeToFirstGate", firstGateM);
  myAttributes->put<std::string>("RangeToFirstGateUnits", "Meters");
}
