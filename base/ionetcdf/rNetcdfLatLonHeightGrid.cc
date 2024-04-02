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
    //    LatLonHeightGridSP->postRead(keys);
    return LatLonHeightGridSP;
  } else {
    return nullptr;
  }
} // NetcdfLatLonHeightGrid::read

bool
NetcdfLatLonHeightGrid::write(std::shared_ptr<DataType> dt,
  std::map<std::string, std::string>                    & keys)
{
  return (NetcdfDataGrid::write(dt, keys));
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
