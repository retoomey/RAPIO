#pragma once

#include <rIONetcdf.h>
#include "rRadialSet.h"
#include "rNetcdfDataGrid.h"

namespace rapio {
/** Handles the read/write of RadialSet DataType from a netcdf file.  */
class NetcdfRadialSet : public NetcdfDataGrid {
public:

  virtual
  ~NetcdfRadialSet();

  /** Initial introduction of NetcdfRadialSet specializer to IONetcdf */
  static void
  introduceSelf(IONetcdf * owner);

  /** Read a RadialSet from NETCDF */
  virtual std::shared_ptr<DataType>
  readNETCDF(int ncid, IOConfig& keys) override;

  /** Write a RadialSet to NETCDF */
  virtual bool
  writeNETCDF(int             ncid,
    std::shared_ptr<DataType> dt,
    IOConfig                  & keys) override;
}
;
}
