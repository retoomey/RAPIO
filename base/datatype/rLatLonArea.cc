#include "rLatLonArea.h"

using namespace rapio;
using namespace std;

LLCoverageArea
LatLonArea::getLLCoverageArea()
{
  const size_t aNumX     = getNumLons(); // X is east to west
  const size_t aNumY     = getNumLats(); // Y is north to south
  const auto l           = getTopLeftLocationAt(0, 0);
  const AngleDegs north  = l.getLatitudeDeg();
  const AngleDegs west   = l.getLongitudeDeg();
  const auto southDelta  = getLatSpacing();
  const auto eastSpacing = getLonSpacing();
  const AngleDegs south  = north - (southDelta * aNumY); // humm messy and/or round off?
  const AngleDegs east   = west + (eastSpacing * aNumX);

  auto a = LLCoverageArea(north, west, south, east, southDelta, eastSpacing, aNumX, aNumY);

  return a;
}

void
LatLonArea::deep_copy(std::shared_ptr<LatLonArea> nsp)
{
  DataGrid::deep_copy(nsp);

  // Copy our stuff
  auto & n = *nsp;

  n.myLatSpacing = myLatSpacing;
  n.myLonSpacing = myLonSpacing;
}

void
LatLonArea::setSpacing(AngleDegs lat_spacing, AngleDegs lon_spacing)
{
  myLatSpacing = round(lat_spacing * 10000) / 10000;
  myLonSpacing = round(lon_spacing * 10000) / 10000;
  bool okLat = (myLatSpacing > 0);
  bool okLon = (myLonSpacing > 0);

  if (!(okLat && okLon)) {
    fLogSevere("*** WARNING *** non-positive element in grid spacing: ({}, {})", lat_spacing, lon_spacing);
  }
}

bool
LatLonArea::initFromGlobalAttributes()
{
  bool success = true;

  DataType::initFromGlobalAttributes();

  // TypeName check, such as Reflectivity or Velocity
  if (myTypeName == "not set") {
    fLogSevere("Missing TypeName attribute such as ReflectivityCONUS.");
    success = false;
  }

  // -------------------------------------------------------
  // Latitude and Longitude grid spacing
  double holderLat, holderLon;

  if (!getDouble("LatGridSpacing", holderLat)) {
    fLogSevere("Missing LatGridSpacing attribute");
    success = false;
  }
  if (!getDouble("LonGridSpacing", holderLon)) {
    fLogSevere("Missing LonGridSpacing attribute");
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

LLH
LatLonArea::getCenterLocation() const
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

std::ostream&
rapio::operator << (std::ostream& os, const LatLonArea& g)
{
  // FIXME: API better? Could just get the LLCoverageArea I think...
  const auto & l        = g.myLocation;
  const AngleDegs north = l.getLatitudeDeg();
  const AngleDegs west  = l.getLongitudeDeg();
  const AngleDegs south = north - (g.myLatSpacing * g.getNumLats());
  const AngleDegs east  = west + (g.myLonSpacing * g.getNumLons());
  const AngleDegs lat   = g.getNumLats();
  const AngleDegs lon   = g.getNumLons();
  const AngleDegs h     = g.getNumLayers();

  os << "nw(" << north << ", " << west << ") "
     << "se(" << south << ", " << east << ") "
     << "s(" << g.myLatSpacing << ", " << g.myLonSpacing << ") "
     << "size(" << lat << " lats, " << lon << " lons, " << h << " layers)";
  return os;
}
