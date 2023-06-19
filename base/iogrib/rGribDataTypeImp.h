#pragma once

#include <rDataType.h>
#include <rDataGrid.h>
#include <rTime.h>
#include <rLLH.h>
#include <rIOGrib.h>
#include <rGribDataType.h>
#include <rOS.h>

#include <rGribAction.h>

#include <memory>

namespace rapio {
/** DataType for holding Grib data
 * Implement the GribDataTypeImp interface from RAPIO
 * to provide Grib2 support.
 * @author Robert Toomey */
class GribDataTypeImp : public GribDataType {
public:
  GribDataTypeImp(const URL& url, const std::vector<char>& buf, int mode) : myURL(url), myBuf(buf), myMode(mode)
  {
    myDataType = "GribData";

    // Figure out the index name of the ".idx" file
    std::string root;
    std::string ext = OS::getRootFileExtension(myURL.toString(), root);

    myIndexURL = URL(root + ".idx");

    // Hack to snag first time.
    // Gonna force message mode here for the time snag
    // Note this means direct URL reading breaks. Need field cleanup
    GribScanFirstMessage scan(this);

    IOGrib::scanGribDataFILE(myURL, &scan);
  }

  /** Call the correct IOGrib scan technique for our mode, and apply the given strategy */
  void
  scanGribData(GribAction * scan);

  /** Print the catalog for this GribDataType. */
  void
  printCatalog();

  /** One way to get 2D data, using key and level string like our HMET library */
  std::shared_ptr<Array<float, 2> >
  getFloat2D(const std::string& key, const std::string& levelstr, size_t&x, size_t&y);

  /** Read the GRIB2 data and put it in a 3-D pointer.
   *    @param key - GRIB2 parameter "TMP"
   *    @param x - if 0 then the dimension's size will be returned, otherwise use the one given
   *    @param y - if 0 then the dimension's size will be returned, otherwise use the one given
   *    @param z - if 0 then the dimension's size will be returned, otherwise use the one given
   *    @param zLevelsVec - a vector of strings containing the levels desired, e.g. ["100 mb", "250 mb"]. An empty string will return all
   *    @param missing - OPTIONAL missing value; default is -999.0
   */
  std::shared_ptr<Array<float, 3> >
  getFloat3D(const std::string& key, std::vector<std::string> zLevelsVec, size_t& x, size_t& y, size_t& z);

private:

  /** Store the URL to the grib2 file locationif any */
  URL myURL;

  /** Store the URL to the index if any */
  URL myIndexURL;

  /** Mode from rapiosettings.xml.  Tells how we buffer grib2 files */
  int myMode;

  /** Store the buffer of the entire grib2 file of data in mode 1
   * Note doing this in mode 1 can be a large amount of RAM
   * for large grib2 files. */
  std::vector<char> myBuf;
};
}
