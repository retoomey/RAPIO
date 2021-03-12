#include "rLatLonGrid.h"
#include "rProject.h"

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

std::shared_ptr<LatLonGrid>
LatLonGrid::Create(
  const std::string& TypeName,
  const std::string& Units,

  // Projection information (metadata of the 2D)
  const LLH        & location,
  const Time       & time,
  const float      lat_spacing,
  const float      lon_spacing,

  // Basically the 2D array and initial value
  size_t           num_lats,
  size_t           num_lons,
  float            value
)
{
  auto llgridsp = std::make_shared<LatLonGrid>();

  if (llgridsp == nullptr) {
    LogSevere("Couldn't create LatLonGrid\n");
  } else {
    // Initialize default band
    LatLonGrid& llgrid = *llgridsp;
    llgrid.init(location, time, lat_spacing, lon_spacing);
    llgrid.setTypeName(TypeName);
    llgrid.setDataAttributeValue("Unit", "dimensionless", Units);

    // Update primary grid to the given size
    llgrid.declareDims({ num_lats, num_lons }, { "Lat", "Lon" });
    std::vector<size_t> dimindexes;
    dimindexes.push_back(0);
    dimindexes.push_back(1);
    llgrid.addFloat2D(TypeName, Units, dimindexes);
  }
  return llgridsp;
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

  // Shared coded with RadialSet.  Gotta be careful moving 'up'
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

  // LatLonGrid only

  // -------------------------------------------------------
  // Latitude and Longitude grid spacing
  if (!getFloat("LatGridSpacing", myLatSpacing)) {
    LogSevere("Missing LatGridSpacing attribute\n");
    success = false;
  }
  if (!getFloat("LonGridSpacing", myLonSpacing)) {
    LogSevere("Missing LonGridSpacing attribute\n");
    success = false;
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
  setDouble("LatGridSpacing", myLatSpacing);
  setDouble("LonGridSpacing", myLonSpacing);
}

double
LatLonGrid::getValueAtLL(double latDegs, double lonDegs, const std::string& layer)
{
  const auto latnw = myLocation.getLatitudeDeg();
  double x         = (latnw - latDegs) / myLatSpacing;

  if (x < 0) { return Constants::MissingData; }

  const auto lonnw = myLocation.getLongitudeDeg();
  double y         = (lonDegs - lonnw) / myLonSpacing;
  if (y < 0) { return Constants::MissingData; }

  const size_t i = (size_t) (abs(x));
  if (i < getNumLats()) {
    const size_t j = (size_t) (abs(y));
    if (j < getNumLons()) {
      auto& data = getFloat2D(layer.c_str())->ref();
      return data[i][j];
    }
  }

  return Constants::MissingData;
}

bool
LatLonGrid::LLCoverageCenterDegree(const float degreeOut, const size_t numRows, const size_t numCols,
  float& topDegs, float& leftDegs, float& deltaLatDegs, float& deltaLonDegs)
{
  // location is the top left, find the absolute center (for x, y > 0)
  const auto lonc = (myLocation.getLongitudeDeg()) + (myLonSpacing * getNumLons() / 2.0);
  const auto latc = (myLocation.getLatitudeDeg()) - (myLatSpacing * getNumLats() / 2.0);

  Project::createLatLonGrid(latc, lonc, degreeOut, numRows, numCols, topDegs, leftDegs, deltaLatDegs, deltaLonDegs);
  return true;
}

bool
LatLonGrid::LLCoverageFull(size_t& numRows, size_t& numCols,
  float& topDegs, float& leftDegs, float& deltaLatDegs, float& deltaLonDegs)
{
  // FIXME: Full is doing angle then back to index which is inefficient
  // I could see this for doing scaling but maybe for a 'full' we have
  // a wrapper of some sort  (we can just loop in x/y)
  numRows      = getNumLats();
  numCols      = getNumLons();
  topDegs      = myLocation.getLatitudeDeg();
  leftDegs     = myLocation.getLongitudeDeg();
  deltaLatDegs = -myLatSpacing;
  deltaLonDegs = myLonSpacing;
  return true;
}
