#pragma once

#include <rIONetcdf.h>

namespace rapio {
class LatLonGrid;

/** Handles the reading/writing of LatLonGrid DataType from a netcdf file.  */
class NetcdfLatLonGrid : public NetcdfType {
public:

  /** Write DataType from given ncid */
  virtual bool
  write(int       ncid,
    const DataType& dt) override;

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

  /*** C write to create a CF header */
  static void
  writeCFHeader(int ncid,
    int             lat_dim,
    int             lon_dim,
    int *           latvar,
    int *           lonvar,
    int *           time_dim,
    int *           time_var);

  /** C write a set of lat/lon degree values to variable given parameters. */
  static void
  writeLatLonValues(
    int          ncid,
    const int    lat_var,
    const int    lon_var,
    const float  oLatDegs,
    const float  oLonDegs,
    const float  dLatDegs,
    const float  dLonDegs,
    const size_t lat_size,
    const size_t lon_size);

  /** C write out a LatLonGrid to ncid. */
  static bool
  write(int     ncid,
    LatLonGrid  & latlon_grid,
    const float missing,
    const float rangeFolded,
    const bool  cdm_compliance = false,
    const bool  faa_compliance = false);

  virtual
  ~NetcdfLatLonGrid();

  static void
  introduceSelf();

private:

  std::string filename;
}
;
}
