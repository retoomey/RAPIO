#include "rNetcdfLatLonGrid.h"

#include "rLatLonGrid.h"
#include "rError.h"
#include "rConstants.h"

using namespace rapio;
using namespace std;

NetcdfLatLonGrid::~NetcdfLatLonGrid()
{ }

void
NetcdfLatLonGrid::introduceSelf(IONetcdf * owner)
{
  std::shared_ptr<IOSpecializer> io = std::make_shared<NetcdfLatLonGrid>();

  owner->introduce("LatLonGrid", io);
  owner->introduce("SparseLatLonGrid", io);
}

std::shared_ptr<DataType>
NetcdfLatLonGrid::read(
  std::map<std::string, std::string>& keys,
  std::shared_ptr<DataType>         dt)
{
  std::shared_ptr<LatLonGrid> LatLonGridSP = std::make_shared<LatLonGrid>();

  if (readDataGrid(LatLonGridSP, keys)) {
    return LatLonGridSP;
  } else {
    return nullptr;
  }
} // NetcdfLatLonGrid::read

bool
NetcdfLatLonGrid::write(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>              & keys)
{
  // Generalize the writer maybe...
  // if (dfs->cdmcompliance || dfs->faacompliance) {
  //  LogSevere("Ignoring cdm/faa flags, need example files for this.\n");
  // }
  // FIXME: Note, we might want to validate the dimensions, etc.
  // Two dimensions: "Lat", "Lon"

  auto * llhg = (LatLonHeightGrid *) (dt.get());

  return (NetcdfDataGrid::write(dt, keys));
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

  auto llgridsp = LatLonGrid::Create(
    "MergedReflectivityQC",
    "MetersPerSecond",
    location,
    time,
    lat_spacing,
    lon_spacing,
    num_lats,
    num_lons);

  return (llgridsp);
}
