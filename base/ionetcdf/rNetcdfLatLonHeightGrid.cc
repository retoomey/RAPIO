#include "rNetcdfLatLonHeightGrid.h"

#include "rLatLonHeightGrid.h"
#include "rError.h"
#include "rConstants.h"

using namespace rapio;
using namespace std;

NetcdfLatLonHeightGrid::~NetcdfLatLonHeightGrid()
{ }

void
NetcdfLatLonHeightGrid::introduceSelf(IONetcdf * owner)
{
  std::shared_ptr<IOSpecializer> io = std::make_shared<NetcdfLatLonHeightGrid>();

  owner->introduce("LatLonHeightGrid", io);
  owner->introduce("SparseLatLonHeightGrid", io);
}

std::shared_ptr<DataType>
NetcdfLatLonHeightGrid::read(
  std::map<std::string, std::string>& keys,
  std::shared_ptr<DataType>         dt)
{
  std::shared_ptr<LatLonHeightGrid> LatLonHeightGridSP = std::make_shared<LatLonHeightGrid>();

  if (readDataGrid(LatLonHeightGridSP, keys)) {
    return LatLonHeightGridSP;
  } else {
    return nullptr;
  }
} // NetcdfLatLonHeightGrid::read

bool
NetcdfLatLonHeightGrid::write(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>                    & keys)
{
  LogInfo("**** ALPHA: Calling netcdf LatLonHeightGrid writer ******\n");

  // Convert current 3D array to pixel sparse format.
  // Also this technique will get sparse out of the netcdf writer if it works.
  // So at moment I've got two competing techniques.  Sparse here in netcdf,
  // and sparse 'outside'. So it's messy at this moment.
  LatLonHeightGrid * LLHG = (LatLonHeightGrid *) (dt.get());

  LLHG->makeSparse(); // 3D to pixels, hiding 3D original from writer
  bool success = NetcdfDataGrid::write(dt, keys);

  LLHG->makeNonSparse(); // Get back the 3D normal array for the caller.

  return success;
} // NetcdfLatLonHeightGrid::write

std::shared_ptr<DataType>
NetcdfLatLonHeightGrid::getTestObject(
  LLH    location,
  Time   time,
  size_t objectNumber)
{
  // Ignore object number we only have 1
  const size_t num_lats = 10;
  const size_t num_lons = 20;
  const size_t num_hts  = 2;
  float lat_spacing     = .05;
  float lon_spacing     = .05;

  auto llgridsp = LatLonHeightGrid::Create(
    "MergedReflectivityQC",
    "MetersPerSecond",
    location,
    time,
    lat_spacing,
    lon_spacing,
    num_lats,
    num_lons,
    num_hts);

  return (llgridsp);
}
