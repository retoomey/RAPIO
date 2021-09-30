#pragma once

#include <rIONetcdf.h>
#include <rNetcdfDataGrid.h>

namespace rapio {
class LatLonGrid;

/** Handles the reading/writing of LatLonGrid DataType from a netcdf file.  */
class NetcdfLatLonGrid : public NetcdfDataGrid {
public:

  /** Write DataType from given ncid */
  virtual bool
  write(
    std::shared_ptr<DataType> dt,
    std::map<std::string, std::string>& keys)
  override;

  /** Read a LatLonGrid with given keys */
  virtual std::shared_ptr<DataType>
  read(
    std::map<std::string, std::string>& keys,
    std::shared_ptr<DataType> dt)
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

  /** C write out a LatLonGrid to ncid. */

  /*static bool
   * write(int     ncid,
   * LatLonGrid  & latlon_grid,
   * const float missing,
   * const float rangeFolded,
   * const bool  cdm_compliance = false,
   * const bool  faa_compliance = false);
   */

  virtual
  ~NetcdfLatLonGrid();

  static void
  introduceSelf(IONetcdf * owner);

private:

  std::string filename;
}
;
}
