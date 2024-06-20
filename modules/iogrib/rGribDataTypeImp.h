#pragma once

#include <rDataType.h>
#include <rDataGrid.h>
#include <rTime.h>
#include <rLLH.h>
#include <rIOGrib.h>
#include <rGribDataType.h>
#include <rOS.h>
#include "rGribDatabase.h"

#include <rGribAction.h>

#include <memory>

namespace rapio {
/** DataType for holding Grib data
 * Implement the GribDataTypeImp interface from RAPIO
 * to provide Grib2 support.
 * @author Robert Toomey */
class GribDataTypeImp : public GribDataType {
public:
  /** Construct a grib2 implementation */
  GribDataTypeImp(const URL& url, const std::vector<char>& buf, int mode);

  /** Call the correct IOGrib scan technique for our mode, and apply the given strategy */
  void
  scanGribData(GribAction * scan);

  /** Print the catalog for this GribDataType. */
  void
  printCatalog();

  /** Get matched message itself by key and level */
  std::shared_ptr<GribMessage>
  getMessage(const std::string& key, const std::string& levelstr);

  /** One way to get 2D data, using key and level string like our HMET library */
  std::shared_ptr<Array<float, 2> >
  getFloat2D(const std::string& key, const std::string& levelstr);

  /** Read the GRIB2 data and put it in a 3-D pointer.
   *    @param key - GRIB2 parameter "TMP"
   *    @param zLevelsVec - a vector of strings containing the levels desired, e.g. ["100 mb", "250 mb"]. An empty string will return all
   */
  std::shared_ptr<Array<float, 3> >
  getFloat3D(const std::string& key, std::vector<std::string> zLevelsVec);

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

  /** Do we have the IDX messages or do we use global catalog? */
  bool myHaveIDX;

  /** Index to IDX records for lookup if available.  Otherwise
   * we fall back to the internal GribDatabase catalog */
  std::vector<GribIDXMessage> myIDXMessages;
};
}
