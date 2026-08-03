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
NetcdfLatLonHeightGrid::readNETCDF(int ncid, IOConfig& keys)
{
  std::shared_ptr<LatLonHeightGrid> LatLonHeightGridSP = std::make_shared<LatLonHeightGrid>();

  if (readDataGrid(ncid, LatLonHeightGridSP, keys)) {
    return LatLonHeightGridSP;
  } else {
    return nullptr;
  }
}

bool
NetcdfLatLonHeightGrid::writeNETCDF(int ncid,
  std::shared_ptr<DataType>             dt,
  IOConfig                              & keys)
{
  return (NetcdfDataGrid::write(dt, keys));
} // NetcdfLatLonHeightGrid::write
