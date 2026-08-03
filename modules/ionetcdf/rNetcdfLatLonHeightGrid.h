#pragma once

#include <rIONetcdf.h>
#include <rNetcdfDataGrid.h>

namespace rapio {
class LatLonHeightGrid;

/** Handles the reading/writing of LatLonHeightGrid DataType from a netcdf file.  */
class NetcdfLatLonHeightGrid : public NetcdfDataGrid {
public:

  virtual
  ~NetcdfLatLonHeightGrid();

  static void
  introduceSelf(IONetcdf * owner);

  /** Read a LatLonHeightGrid from NETCDF */
  virtual std::shared_ptr<DataType>
  readNETCDF(int ncid, IOConfig& keys) override;

  /** Write a LatLonHeightGrid to NETCDF */
  virtual bool
  writeNETCDF(int             ncid,
    std::shared_ptr<DataType> dt,
    IOConfig                  & keys) override;
}
;
}
