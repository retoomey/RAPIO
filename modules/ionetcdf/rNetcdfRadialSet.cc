#include "rNetcdfRadialSet.h"

#include "rRadialSet.h"
#include "rError.h"
#include "rUnit.h"
#include "rConstants.h"

#include <netcdf.h>

#include <rDataGrid.h> // macros

using namespace rapio;
using namespace std;

NetcdfRadialSet::~NetcdfRadialSet()
{ }

void
NetcdfRadialSet::introduceSelf(IONetcdf * owner)
{
  std::shared_ptr<IOSpecializer> io = std::make_shared<NetcdfRadialSet>();

  // NOTE: We read in polar grids and turn them into full radialsets
  owner->introduce("RadialSet", io);
  owner->introduce("PolarGrid", io);
  owner->introduce("SparseRadialSet", io);
  owner->introduce("SparsePolarGrid", io);
}

std::shared_ptr<DataType>
NetcdfRadialSet::readNETCDF(int ncid, IOConfig& keys)
{
  std::shared_ptr<RadialSet> radialSetSP = std::make_shared<RadialSet>();

  if (readDataGrid(ncid, radialSetSP, keys)) {
    return radialSetSP;
  } else {
    return nullptr;
  }
}

bool
NetcdfRadialSet::writeNETCDF(int ncid,
  std::shared_ptr<DataType>      dt,
  IOConfig                       & keys)
{
  return (NetcdfDataGrid::writeNETCDF(ncid, dt, keys));
}
