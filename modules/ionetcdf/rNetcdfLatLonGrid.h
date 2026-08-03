#pragma once

#include <rIONetcdf.h>
#include <rNetcdfDataGrid.h>

namespace rapio {
class LatLonGrid;

/** Handles the reading/writing of LatLonGrid DataType from a netcdf file.  */
class NetcdfLatLonGrid : public NetcdfDataGrid {
public:

  virtual
  ~NetcdfLatLonGrid();

  /** Initial introduction to IONetcdf */
  static void
  introduceSelf(IONetcdf * owner);

  /** Read a LatLonGrid from NETCDF */
  virtual std::shared_ptr<DataType>
  readNETCDF(int ncid, IOConfig& keys) override;

  /** Write a LatLonGrid to NETCDF */
  virtual bool
  writeNETCDF(int             ncid,
    std::shared_ptr<DataType> dt,
    IOConfig                  & keys) override;
}
;
}
