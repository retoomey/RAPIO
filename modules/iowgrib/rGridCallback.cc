#include "rGridCallback.h"
#include "rURL.h"
#include "rError.h"
#include "rIOWgrib.h"
#include "rLatLonGrid.h"

#include <sstream>

using namespace rapio;

std::shared_ptr<LatLonGrid> GridCallback::myTempLatLonGrid;

GridCallback::GridCallback(const URL& u, const std::string& match, const std::string& dkey)
  : WgribCallback(u, match, dkey)
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
GridCallback::handleInitialize(int * decode, int * latlon)
{
  //std::cout << "[C++] Initialize grid\n";
  // For grids, we want decoding/unpacking of data
  *decode = 1;
  // For grids, we want the lat lon values
  *latlon = 1;
}

void
GridCallback::handleFinalize()
{
  //std::cout << "[C++] Finalize grid\n";
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

  // Create a bounding box from the passed in wgrib2 arrays
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
GridCallback::handleSetDataArray(float * data, int nlats, int nlons, unsigned int * index)
{
  if ((data == nullptr) || (index == nullptr)) {
    std::cout << "[C++} Data index is null, can't fill output grid.\n";
    return;
  }

  const auto npoints = nlats * nlons;

  // Create a LatLonGrid and fill in the projected array.
  // FIXME: 3D will have to be a bit differently done
  LLCoverageArea a = getLLCoverageArea();

  myTempLatLonGrid = LatLonGrid::Create(
    "SetMyTypeName", // FIXME: from data somehow? Or caller sets?
    "SetMyUnits",
    getTime(),
    a);

  auto& array   = myTempLatLonGrid->getFloat2DRef();
  auto * output = array.data();

  for (int i = 0; i < npoints; i++) {
    const int idx = index[i];

    // 0 or negative index means data unavailable
    if (idx <= 0) {
      // output[i] = 300.0f; // Unavailable
      output[i] = Constants::DataUnavailable;
    } else {
      float& value = data[idx - 1]; // wgrib2 is 1-based

      // Optionally check for NaN or sentinel inside data
      if (std::isnan(value)) {
        // output[i] = -99900.0f; // Missing
        output[i] = Constants::MissingData;
      } else {
        output[i] = value;
      }
    }
  }
} // GridCallback::handleSetDataArray
