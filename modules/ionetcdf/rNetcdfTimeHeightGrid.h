#pragma once
#include <rIONetcdf.h>
#include <rNetcdfDataGrid.h>
#include "rTimeHeightGrid.h"

namespace rapio {

class NetcdfTimeHeightGrid : public NetcdfDataGrid {
public:
  virtual ~NetcdfTimeHeightGrid();

  static void
  introduceSelf(IONetcdf * owner);

  virtual std::shared_ptr<DataType>
  readNETCDF(int ncid, IOConfig& keys) override;

  virtual bool
  writeNETCDF(int ncid, std::shared_ptr<DataType> dt, IOConfig& keys) override;
};

} // namespace rapio
