#include "rNetcdfTimeHeightGrid.h"

using namespace rapio;

NetcdfTimeHeightGrid::~NetcdfTimeHeightGrid() 
{ }

void 
NetcdfTimeHeightGrid::introduceSelf(IONetcdf * owner) 
{
  std::shared_ptr<IOSpecializer> newOne = std::make_shared<NetcdfTimeHeightGrid>();
  owner->introduce("TimeHeightGrid", newOne);
}

std::shared_ptr<DataType> 
NetcdfTimeHeightGrid::readNETCDF(int ncid, IOConfig& keys) 
{
  // Allocate the specific derived class
  auto grid = std::make_shared<TimeHeightGrid>();

  // Use the base class logic to read dimensions, variables, and attributes
  if (readDataGrid(ncid, grid, keys)) {
    return grid;
  }
  
  return nullptr;
}

bool 
NetcdfTimeHeightGrid::writeNETCDF(int ncid, std::shared_ptr<DataType> dt, IOConfig& keys) 
{
  // The base NetcdfDataGrid writer already knows how to write out generic dimensions, 
  // 1D coordinates, and 2D variables, so we can just pass it up the chain.
  return NetcdfDataGrid::writeNETCDF(ncid, dt, keys);
}
