#include "rNetcdfLatLonGrid.h"

#include "rLatLonGrid.h"
#include "rError.h"
#include "rConstants.h"
#include "rProcessTimer.h"

using namespace rapio;
using namespace std;

NetcdfLatLonGrid::~NetcdfLatLonGrid()
{ }

void
NetcdfLatLonGrid::introduceSelf()
{
  std::shared_ptr<NetcdfType> io = std::make_shared<NetcdfLatLonGrid>();
  IONetcdf::introduce("LatLonGrid", io);
  IONetcdf::introduce("SparseLatLonGrid", io);
}

std::shared_ptr<DataType>
NetcdfLatLonGrid::read(const int ncid, const URL& loc,
  const vector<string>& params)
{
  std::shared_ptr<LatLonGrid> LatLonGridSP = std::make_shared<LatLonGrid>();
  if (readDataGrid(ncid, LatLonGridSP, loc, params)) {
    return LatLonGridSP;
  } else {
    return nullptr;
  }
} // NetcdfLatLonGrid::read

bool
NetcdfLatLonGrid::write(int ncid, std::shared_ptr<DataType> dt,
  std::shared_ptr<DataFormatSetting> dfs)
{
  // Generalize the writer maybe...
  if (dfs->cdmcompliance || dfs->faacompliance) {
    LogSevere("Ignoring cdm/faa flags, need example files for this.\n");
  }
  // FIXME: Note, we might want to validate the dimensions, etc.
  // Two dimensions: "Lat", "Lon"
  return (NetcdfDataGrid::write(ncid, dt, dfs));
} // NetcdfLatLonGrid::write

std::shared_ptr<DataType>
NetcdfLatLonGrid::getTestObject(
  LLH    location,
  Time   time,
  size_t objectNumber)
{
  // Ignore object number we only have 1
  const size_t num_lats = 10;
  const size_t num_lons = 20;
  float lat_spacing     = .05;
  float lon_spacing     = .05;

  std::shared_ptr<LatLonGrid> llgridsp = std::make_shared<LatLonGrid>(
    location, time,
    lat_spacing,
    lon_spacing);
  LatLonGrid& llgrid = *llgridsp;

  // llgrid.resize(num_lats, num_lons, 7.0f);
  llgrid.declareDims({ num_lats, num_lons }, { "Lat", "Lon" });

  llgrid.setTypeName("MergedReflectivityQC");

  // RAPIO: Direct access into lat lon grid...
  llgrid.setDataAttributeValue("Unit", "dimensionless", "MetersPerSecond");

  return (llgridsp);
}
