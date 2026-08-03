#pragma once

#include <rIONetcdf.h>
#include "rDataGrid.h"
#include <rNetcdfSpecializer.h>
#include "rURL.h"

namespace rapio {
/** Handles the read/write of DataGrid DataType from a netcdf file.
 * @author Robert Toomey
 */
class NetcdfDataGrid : public NetcdfSpecializer {
public:

  virtual
  ~NetcdfDataGrid();

  /** Initial introduction to IONetcdf */
  static void
  introduceSelf(IONetcdf * owner);

  /** Read a DataType from NETCDF */
  virtual std::shared_ptr<DataType>
  readNETCDF(int ncid, IOConfig& keys) override;

  /** Write a DataType to NETCDF */
  virtual bool
  writeNETCDF(int             ncid,
    std::shared_ptr<DataType> dt,
    IOConfig                  & keys) override;

protected:

  /** Lower level utility to read generically into a already created
   * DataGrid */
  virtual bool
  readDataGrid(
    int                       ncid,
    std::shared_ptr<DataGrid> dt,
    IOConfig                  & keys);
}
;
}
