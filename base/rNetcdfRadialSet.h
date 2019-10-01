#pragma once

#include <rIONetcdf.h>
#include "rRadialSet.h"
#include "rURL.h"

namespace rapio {
/** Handles the read/write of RadialSet DataType from a netcdf file.  */
class NetcdfRadialSet : public NetcdfType {
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

  /** C based creation method */
  static std::shared_ptr<DataType>
  read(const int                  ncid,
    const std::vector<std::string>& params);

  /** C write out a RadialSet to ncid. */
  static bool
  write(int     ncid,
    RadialSet   & radialSet,
    const float missing,
    const float rangeFolded);

  /** Write DataType from given ncid */
  virtual bool
  write(int                            ncid,
    const DataType                     & dt,
    std::shared_ptr<DataFormatSetting> dfs)
  override;

  virtual
  ~NetcdfRadialSet();

  static void
  introduceSelf();
}
;
}
