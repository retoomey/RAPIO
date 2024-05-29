#pragma once

#include <rIONetcdf.h>
#include "rRadialSet.h"
#include "rNetcdfDataGrid.h"
#include "rURL.h"

namespace rapio {
/** Handles the read/write of RadialSet DataType from a netcdf file.  */
class NetcdfRadialSet : public NetcdfDataGrid {
public:

  /** Read a RadialSet with given keys */
  virtual std::shared_ptr<DataType>
  read(
    std::map<std::string, std::string>& keys,
    std::shared_ptr<DataType>         dt)
  override;

  /** Write DataType from given ncid */
  virtual bool
  write(
    std::shared_ptr<DataType>         dt,
    std::map<std::string, std::string>& keys)
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

  /** Initial introduction of NetcdfRadialSet specializer to IONetcdf */
  static void
  introduceSelf(IONetcdf * owner);
}
;
}
