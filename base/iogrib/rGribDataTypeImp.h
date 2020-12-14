#pragma once

#include <rDataType.h>
#include <rDataGrid.h>
#include <rTime.h>
#include <rLLH.h>
#include <rIOGrib.h>
#include <rGribDataType.h>

#include <memory>

namespace rapio {
/** DataType for holding Grib data
 * Implement the GribDataTypeImp interface from RAPIO
 * to do provide Grib2 support.
 * @author Robert Toomey */
class GribDataTypeImp : public GribDataType {
public:
  GribDataTypeImp(const std::vector<char>& buf) : myBuf(buf)
  {
    myDataType = "GribData";
  }

  /** Print the catalog for thie GribDataType.
   * FIXME: better to be able to get a catalog I think */
  void
  printCatalog();

  /** One way to get 2D data, using key and level string like our HMET library */
  std::shared_ptr<Array<float, 2> >
  getFloat2D(const std::string& key, const std::string& levelstr, size_t&x, size_t&y);

  /** Read the GRIB2 data and put it in a 3-D pointer.
   *    @param key - GRIB2 parameter "TMP"
   *    @param levelstr -
   *    @param x - if 0 then the dimension's size will be returned, otherwise use the one given
   *    @param y - if 0 then the dimension's size will be returned, otherwise use the one given
   *    @param z - if 0 then the dimension's size will be returned, otherwise use the one given
   *    @param zLevelsVec - a vector of strings containing the levels desired, e.g. ["100 mb", "250 mb"]. An empty string will return all
   *    @param missing - OPTIONAL missing value; default is -999.0
   */
  std::shared_ptr<Array<float, 3> >
  getFloat3D(const std::string& key, size_t& x, size_t& y, size_t& z, std::vector<std::string> zLevelsVec,
    float missing = -999.0);


private:
  /** Store the buffer of data (copy wraps around shared_ptr) */
  std::vector<char> myBuf;
};
}
