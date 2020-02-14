#include "rLatLonGrid.h"

using namespace rapio;
using namespace std;

std::string
LatLonGrid::getGeneratedSubtype() const
{
  return (formatString(myLocation.getHeightKM(), 5, 2));
}

LatLonGrid::LatLonGrid()
{
  myDataType = "LatLonGrid";
  // init(LLH(), Time::CurrentTime(), 1, 1);
}

LatLonGrid::LatLonGrid(

  // Projection information (metadata of the 2D)
  const LLH   & location,
  const Time  & time,
  const float lat_spacing, // FIXME: stored as double
  const float lon_spacing,

  // Basically the 2D array and initial value
  size_t      rows,
  size_t      cols,
  float       value
)
{
  // Lookup for read/write factories
  myDataType = "LatLonGrid";

  init(location, time, lat_spacing, lon_spacing);
}

/** Set what defines the lat lon grid in spacetime */
void
LatLonGrid::init(
  const LLH& location,
  const Time& time,
  const float lat_spacing, const float lon_spacing)
{
  myLocation   = location;
  myTime       = time;
  myLatSpacing = lat_spacing;
  myLonSpacing = lon_spacing;

  bool okLat = (myLatSpacing > 0);
  bool okLon = (myLonSpacing > 0);

  if (!(okLat && okLon)) {
    LogSevere("*** WARNING *** non-positive element in grid spacing\n"
      << "(" << lat_spacing << "," << lon_spacing << ")\n");
  }
}

bool
LatLonGrid::initFromGlobalAttributes()
{
  bool success = true;

  // TypeName check, such as Reflectivity or Velocity
  auto aTypeName = myAttributes.get<std::string>(Constants::TypeName);

  if (!aTypeName) {
    LogSevere("Missing TypeName attribute such as Reflectivity.\n");
    success = false;
  } else {
    myTypeName = *aTypeName;
  }

  // -------------------------------------------------------
  // Location
  auto lat = myAttributes.get<double>(Constants::Latitude);
  if (!lat) { success = false; }
  auto lon = myAttributes.get<double>(Constants::Longitude);
  if (!lon) { success = false; }
  auto ht = myAttributes.get<double>(Constants::Height);
  if (!ht) { success = false; }
  if (success) {
    myLocation = LLH(*lat, *lon, *ht / 1000.0); // diff radial
  } else {
    LogSevere("Missing Location attribute\n");
  }

  // -------------------------------------------------------
  // Time
  auto timesecs = myAttributes.get<long>(Constants::Time);
  if (timesecs) {
    double f        = 0.0;
    auto fractional = myAttributes.get<double>(Constants::FractionalTime);
    if (fractional) {
      f = *fractional;
    }
    // Cast long to time_t..hummm
    myTime = Time(*timesecs, f);
  } else {
    LogSevere("Missing Time attribute\n");
    success = false;
  }

  // LatLonGrid only

  // -------------------------------------------------------
  // Latitude grid spacing
  auto latSpacing = myAttributes.get<double>("LatGridSpacing");
  if (!latSpacing) {
    LogSevere("Missing LatGridSpacing attribute\n");
    success = false;
  } else {
    myLatSpacing = *latSpacing;
  }

  // -------------------------------------------------------
  // Longitude grid spacing
  auto lonSpacing = myAttributes.get<double>("LonGridSpacing");
  if (!lonSpacing) {
    LogSevere("Missing LonGridSpacing attribute\n");
    success = false;
  } else {
    myLonSpacing = *lonSpacing;
  }

  return success;
} // LatLonGrid::initFromGlobalAttributes

void
LatLonGrid::updateGlobalAttributes(const std::string& encoded_type)
{
  // Note: Datatype updates the attributes -unit -value specials,
  // so don't add any after this
  DataType::updateGlobalAttributes(encoded_type);

  // LatLonGrid only global attributes
  myAttributes.put<double>("LatGridSpacing", myLatSpacing);
  myAttributes.put<double>("LonGridSpacing", myLonSpacing);
}
