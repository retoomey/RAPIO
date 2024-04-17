#include "rLatLonGrid.h"
#include "rProject.h"

#include "rProcessTimer.h"

using namespace rapio;
using namespace std;

std::string
LatLonGrid::getGeneratedSubtype() const
{
  return (formatString(myLocation.getHeightKM(), 5, 2));
}

LatLonGrid::LatLonGrid()
{
  setDataType("LatLonGrid");
}

bool
LatLonGrid::init(
  const std::string & TypeName,
  const std::string & Units,
  const LLH         & location,
  const Time        & datatime,
  const float       lat_spacing,
  const float       lon_spacing,
  size_t            num_lats,
  size_t            num_lons
)
{
  DataGrid::init(TypeName, Units, location, datatime, { num_lats, num_lons }, { "Lat", "Lon" });

  setDataType("LatLonGrid");

  setSpacing(lat_spacing, lon_spacing);

  addFloat2D(Constants::PrimaryDataName, Units, { 0, 1 });

  // Store a single static layer matching height in meters
  myLayerNumbers.push_back(location.getHeightKM() * 1000.0);
  return true;
}

std::shared_ptr<LatLonGrid>
LatLonGrid::Create(
  const std::string& TypeName,
  const std::string& Units,

  // Projection information (metadata of the 2D)
  const LLH   & location,
  const Time  & time,
  const float lat_spacing,
  const float lon_spacing,

  // Basically the 2D array
  size_t num_lats,
  size_t num_lons
)
{
  auto newonesp = std::make_shared<LatLonGrid>();

  newonesp->init(TypeName, Units, location, time, lat_spacing, lon_spacing, num_lats, num_lons);
  return newonesp;
}

void
LatLonArea::setSpacing(AngleDegs lat_spacing, AngleDegs lon_spacing)
{
  myLatSpacing = round(lat_spacing * 10000) / 10000;
  myLonSpacing = round(lon_spacing * 10000) / 10000;
  bool okLat = (myLatSpacing > 0);
  bool okLon = (myLonSpacing > 0);

  if (!(okLat && okLon)) {
    LogSevere("*** WARNING *** non-positive element in grid spacing\n"
      << "(" << lat_spacing << "," << lon_spacing << ")\n");
  }
}

bool
LatLonArea::initFromGlobalAttributes()
{
  bool success = true;

  DataType::initFromGlobalAttributes();

  // TypeName check, such as Reflectivity or Velocity
  if (myTypeName == "not set") {
    LogSevere("Missing TypeName attribute such as ReflectivityCONUS.\n");
    success = false;
  }

  // -------------------------------------------------------
  // Latitude and Longitude grid spacing
  double holderLat, holderLon;

  if (!getDouble("LatGridSpacing", holderLat)) {
    LogSevere("Missing LatGridSpacing attribute\n");
    success = false;
  }
  if (!getDouble("LonGridSpacing", holderLon)) {
    LogSevere("Missing LonGridSpacing attribute\n");
    success = false;
  }
  if (success) {
    setSpacing(holderLat, holderLon);
  }

  return success;
} // LatLonArea::initFromGlobalAttributes

void
LatLonArea::updateGlobalAttributes(const std::string& encoded_type)
{
  // Note: Datatype updates the attributes -unit -value specials,
  // so don't add any after this
  DataType::updateGlobalAttributes(encoded_type);

  // LatLonGrid only global attributes
  // Make sure no matter the AngleDeg type, we make sure the double
  // stored precision is good
  double lat_spacing = myLatSpacing;

  lat_spacing = round(lat_spacing * 10000.0) / 10000.0;
  double lon_spacing = myLonSpacing;

  lon_spacing = round(lon_spacing * 10000.0) / 10000.0;
  setDouble("LatGridSpacing", lat_spacing);
  setDouble("LonGridSpacing", lon_spacing);
}

std::shared_ptr<DataProjection>
LatLonGrid::getProjection(const std::string& layer)
{
  // FIXME: Caching now so first layer wins.  We'll need something like
  // setLayer on the projection I think 'eventually'.  We're only using
  // primary layer at moment
  if (myDataProjection == nullptr) {
    myDataProjection = std::make_shared<LatLonGridProjection>(layer, this);
  }
  return myDataProjection;
}

LLH
LatLonArea::getCenterLocation()
{
  // This simple one liner doesn't work, because the middle can be on a cell wall and
  // not in the center of the cell.  Imagine 2 cells..the true middle is the line
  // between them.  For three cells it is the middle of cell 1.
  // return(getCenterLocationAt(getNumLats()/2, getNumLons()/2);
  //
  // However, this does:
  const double latHalfWidth = (myLatSpacing * getNumLats()) / 2.0;
  const double lonHalfWidth = (myLonSpacing * getNumLons()) / 2.0;
  const double latDegs      = myLocation.getLatitudeDeg() - latHalfWidth;
  const double lonDegs      = myLocation.getLongitudeDeg() + lonHalfWidth;

  return LLH(latDegs, lonDegs, myLocation.getHeightKM());
}

LLH
LatLonArea::getTopLeftLocationAt(size_t i, size_t j)
{
  if (i == j == 0) { return myLocation; }
  const double latDegs = myLocation.getLatitudeDeg() - (myLatSpacing * i);
  const double lonDegs = myLocation.getLongitudeDeg() + (myLonSpacing * j);

  return LLH(latDegs, lonDegs, myLocation.getHeightKM());
}

LLH
LatLonArea::getCenterLocationAt(size_t i, size_t j)
{
  const double latDegs = myLocation.getLatitudeDeg() - (myLatSpacing * (i + 0.5));
  const double lonDegs = myLocation.getLongitudeDeg() + (myLonSpacing * (j + 0.5));

  return LLH(latDegs, lonDegs, myLocation.getHeightKM());
}

void
LatLonGrid::postRead(std::map<std::string, std::string>& keys)
{
  // For now, we always unsparse to full.  Though say in rcopy we
  // would want to keep it sparse.  FIXME: have a key control this
  unsparse2D(getNumLats(), getNumLons(), keys);
} // LatLonGrid::postRead

void
LatLonGrid::preWrite(std::map<std::string, std::string>& keys)
{
  sparse2D(); // Standard sparse of primary data (add dimension)
}

void
LatLonGrid::postWrite(std::map<std::string, std::string>& keys)
{
  unsparseRestore();
}
