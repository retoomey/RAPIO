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
NetcdfLatLonGrid::readNETCDF(int ncid, IOConfig& keys)
{
  std::shared_ptr<LatLonGrid> LatLonGridSP = std::make_shared<LatLonGrid>();

  if (readDataGrid(ncid, LatLonGridSP, keys)) {
    return LatLonGridSP;
  } else {
    return nullptr;
  }
}

bool
NetcdfLatLonGrid::writeNETCDF(int ncid,
  std::shared_ptr<DataType>       dt,
  IOConfig                        & keys)
{
  return (NetcdfDataGrid::writeNETCDF(ncid, dt, keys));
} // NetcdfLatLonGrid::write
