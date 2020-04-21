#pragma once

#include <rIONetcdf.h>
#include "rRadialSet.h"
#include "rNetcdfDataGrid.h"
#include "rURL.h"

namespace rapio {
/** Handles the read/write of RadialSet DataType from a netcdf file.  */
// class NetcdfRadialSet : public NetcdfType {
class NetcdfRadialSet : public NetcdfDataGrid {
public:

  /** The way to obtain the object.
   *  @params ncfile An open NetcdfFile object.
   *  prms   Only the file name (first param) is needed.
   */
  virtual std::shared_ptr<DataType>
  read(const int ncid,
    const URL    & loc)
  override;

  /** Write DataType from given ncid */
  virtual bool
  write(int                            ncid,
    std::shared_ptr<DataType>          dt,
    std::shared_ptr<DataFormatSetting> dfs)
  override;

  /** Get number of test objects we provide for read/write tests */
  static size_t
  getTestObjectCount()
  {
    return (1);
  } // RadialSet

  /** Get the ith test object */
  static std::shared_ptr<DataType>
  getTestObject(
    LLH    location,
    Time   time,
    size_t objectNumber);

  virtual
  ~NetcdfRadialSet();

  static void
  introduceSelf();
}
;
}
