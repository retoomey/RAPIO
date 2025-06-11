#include "rGridCallback.h"
#include "rURL.h"
#include "rError.h"
#include "rIOWgrib.h"
#include "rLatLonGrid.h"

#include <sstream>

using namespace rapio;

std::shared_ptr<LatLonGrid> GridCallback::myTempLatLonGrid;

GridCallback::GridCallback(const URL& u) : WgribCallback(u)
{
  // A default lat lon coverage for the moment.
  // FIXME: Probably just conus to start?
  double lat_start = 90.0;
  double lon_start = -220.0;
  double dlat      = 0.05;
  double dlon      = 0.05;
  int nlat         = 2000;
  int nlon         = 4300;

  // Calculate.  LLCoverageArea should do this actually (have two methods,
  // passing sizes and passing coordinates and autosizing.)
  double lat_end = lat_start - (dlat * nlat * 1.0);
  double lon_end = lon_start + (dlon * nlon * 1.0);

  // X and Y are confusing.  X is marching in lon, Y is marching in lat.
  myLLCoverageArea = LLCoverageArea(lat_start, lon_start, lat_end, lon_end, dlat, dlon, nlon, nlat);
}

void
GridCallback::execute()
{
  RAPIOCallbackCPP catalog(this);

  // convert pointer to integer and stream to string
  // string now holds a decimal address
  std::ostringstream oss;

  oss << reinterpret_cast<uintptr_t>(&catalog);
  std::string pointer = oss.str();

  // Catalog doesn't 'really' need a callback, but seems cleaner this way
  const char * argv[] = {
    "wgrib2",
    myFilename.c_str(), // in scope
    // "-match", ":TMP:850 mb:", // Note: Don't add quotes they will be part of the match
    "-match", ":TMP:2 m above ground:", // Ground should match the us map if projection good.
    // "-match", ":TMP:", // Tell wgrib2 what to match.  Note, no quotes here.
    "-rapio",       // Use our callback patch
    pointer.c_str() // Send address of our RapioCallback class.
  };
  int argc = sizeof(argv) / sizeof(argv[0]);

  std::ostringstream param;

  for (size_t i = 0; i < argc; ++i) {
    param << argv[i];
    if (i < argc - 1) { param << " "; }
  }
  LogInfo("Calling: " << param.str() << "\n");

  std::vector<std::string> result = IOWgrib::capture_vstdout_of_wgrib2(argc, argv);

  for (size_t i = 0; i < result.size(); ++i) {
    LogInfo("[wgrib2] " << result[i] << "\n");
  }
} // GridCallback::execute

void
GridCallback::handleInitialize(int * decode, int * latlon)
{
  std::cout << "[C++] Initialize grid callback\n";
  // For grids, we want decoding/unpacking of data
  *decode = 1;
  // For grids, we want the lat lon values
  *latlon = 1;
}

void
GridCallback::handleFinalize()
{
  std::cout << "[C++] Finalize grid callback\n";
}

void
GridCallback::handleSetLatLon(double * lat, double * lon, size_t nx, size_t ny)
{
  std::cout << "[C++] Handle Set Lat Lon called\n";
  // Need at least 2 in each dimension to get deltas
  if ((nx < 2) || (ny < 2)) {
    std::cout << "[C++] Grid too small to guess bounding box\n";
    return;
  }

  // Degrees from wgrib
  double minLat, minLon, maxLat, maxLon;

  const size_t count = nx * ny;

  for (size_t i = 0; i < count; ++i) {
    if (i == 0) {
      minLat = lat[0];
      maxLat = lat[0];
      minLon = lon[0];
      maxLon = lon[0];
    }
    if (lat[i] < minLat) { minLat = lat[i]; }
    if (lat[i] > maxLat) { maxLat = lat[i]; }
    if (lon[i] < minLon) { minLon = lon[i]; }
    if (lon[i] > maxLon) { maxLon = lon[i]; }
  }

  size_t nlon = nx;
  size_t nlat = ny;

  // at top left of cell for min and max
  double dlat = (maxLat - minLat) / nlat;
  double dlon = (maxLon - minLon) / nlon;

  // Our grid wants the actual outside location, so add one dlat and dlon
  minLat -= dlat; // move over to include the final cell
  maxLon += dlon;
  nlon++; // and add a cell.
  nlat++;

  std::cout << "[C++] Estimating mrms lat lon coverage\n";
  myLLCoverageArea = LLCoverageArea(maxLat, minLon, minLat, maxLon, dlat, dlon, nlon, nlat);
} // GridCallback::handleSetLatLon

void
GridCallback::handleGetLLCoverageArea(double * nwLat, double * nwLon,
  double * seLat, double * seLon, double * dLat, double * dLon,
  int * nLat, int * nLon)
{
  std::cout << "[C++] Handle Get LL Coverage Area\n";
  // Send coverage area to C
  LLCoverageArea a = getLLCoverageArea();

  *nwLat = a.getNWLat();
  *nwLon = a.getNWLon();
  *seLat = a.getSELat();
  *seLon = a.getSELon();
  *dLat  = a.getLatSpacing();
  *dLon  = a.getLonSpacing();
  *nLon  = a.getNumX(); // related to X  FIXME: API naming could be better
  *nLat  = a.getNumY(); // related to Y
}

void
GridCallback::handleData(const float * data, int n)
{
  // Since this is called by wgrib2, any output get captured
  // by our wrapper, thus we don't call Log here but std::cout
  std::cout << "\n[C++] Got " << n << " values from GRIB2\n";
  std::cout << "[C++] ";
  for (size_t i = 0; i < 10; ++i) {
    std::cout << data[i];
    if (i < 10 - 1) { std::cout << ","; }
  }
  std::cout << "\n";

  LLCoverageArea a = getLLCoverageArea();

  myTempLatLonGrid = LatLonGrid::Create(
    "Temperature",
    "K",
    Time(),
    a);

  auto& array = myTempLatLonGrid->getFloat2DRef();

  // wgrib2 should at least fill our array directly. We shouldn't
  // have to transform y or anything here.
  // There are way too many arrays copying and projection and
  // indexing, etc.  But baby steps get it working first.
  // FIXME: Pass the array.data() point into the C.  It's a boost
  // array, but the memory should be correct.
  std::memcpy(array.data(), data, n * sizeof(float));
  std::cout << "-----> Done Setting LatLonGrid \n";
}
