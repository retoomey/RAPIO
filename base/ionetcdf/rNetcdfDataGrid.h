#pragma once

#include <rIONetcdf.h>
#include "rDataGrid.h"
#include "rURL.h"

namespace rapio {
/** Handles the read/write of DataGrid DataType from a netcdf file.
 * @author Robert Toomey
 */
class NetcdfDataGrid : public IOSpecializer {
public:

  /** The way to obtain the object.
   *  @params ncfile An open NetcdfFile object.
   *  prms   Only the file name (first param) is needed.
   */
  virtual std::shared_ptr<DataType>
  read(
    std::map<std::string, std::string>& keys)
  override;

  /** Write DataType from given ncid */
  virtual bool
  write(
    std::shared_ptr<DataType> dt,
    std::map<std::string, std::string>& keys)
  override;

  /** Lower level utility to read generically into a already created
   * DataGrid */
  virtual bool
  readDataGrid(
    std::shared_ptr<DataGrid> dt,
    std::map<std::string, std::string>& keys);

  virtual
  ~NetcdfDataGrid();

  /** Initial introduction of NetcdfDataGrid specializer to IONetcdf */
  static void
  introduceSelf(IONetcdf * owner);
}
;
}
