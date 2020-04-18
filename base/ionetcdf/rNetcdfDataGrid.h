#pragma once

#include <rIONetcdf.h>
#include "rDataGrid.h"
#include "rURL.h"

namespace rapio {
/** Handles the read/write of DataGrid DataType from a netcdf file.
 * @author Robert Toomey
 */
class NetcdfDataGrid : public NetcdfType {
public:

  /** The way to obtain the object.
   *  @params ncfile An open NetcdfFile object.
   *  prms   Only the file name (first param) is needed.
   */
  virtual std::shared_ptr<DataType>
  read(const int ncid,
    const URL    & loc,
    const std::vector<std::string>&)
  override;

  /** Lower level utility to read generically into a already created
   * DataGrid */
  virtual bool
  readDataGrid(const int      ncid,
    std::shared_ptr<DataGrid> dt,
    const URL                 & loc,
    const std::vector<std::string>&);

  /** Write DataType from given ncid */
  virtual bool
  write(int                            ncid,
    std::shared_ptr<DataType>          dt,
    std::shared_ptr<DataFormatSetting> dfs)
  override;

  virtual
  ~NetcdfDataGrid();

  static void
  introduceSelf();
}
;
}
