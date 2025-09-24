#pragma once

#include <rDataType.h>
#include <rDataGrid.h>
#include <rTime.h>
#include <rLLH.h>
#include <rIOGrib.h>
#include <rGribDataType.h>
#include <rOS.h>

#include "rGribDatabase.h"
#include "rGribPointerHolder.h"
#include "rGribAction.h"

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
  getMessage(const std::string& key, const std::string& levelstr, const std::string& substr = "");

  /** One way to get 2D data, using key and level string like our HMET library */
  std::shared_ptr<Array<float, 2> >
  getFloat2D(const std::string& key, const std::string& levelstr, const std::string& substr = "");

  /** Read the GRIB2 data and put it in a 3-D pointer.
   *    @param key - GRIB2 parameter "TMP"
   *    @param zLevelsVec - a vector of strings containing the levels desired, e.g. ["100 mb", "250 mb"]. An empty string will return all
   */
  std::shared_ptr<Array<float, 3> >
  getFloat3D(const std::string& key, std::vector<std::string> zLevelsVec);

  /** Get a projected LatLonGrid from the grib data */
  std::shared_ptr<LatLonGrid>
  getLatLonGrid(const std::string& key, const std::string& levelstr, const std::string& substr = "");

  /** Get our validity shared_ptr (only exists if we do)  This is passed to created messages/fields
   * so they can weak reference it. */
  std::shared_ptr<GribPointerHolder> getValidityPtr(){ return myValidPtr; }

  // Access IDX if available

  /** Get product name of field and message number given from our IDX database */
  bool
  getIDXProductName(size_t message, size_t field, std::string& product);

  /** Get level name of field and message number given from our IDX database */
  bool
  getIDXLevelName(size_t message, size_t field, std::string& level);

private:

  /** Get message/field of our IDX storage, if exists */
  bool
  getIDXField(size_t message, size_t field, GribIDXField& out);

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

  /** A dummy std::shared ptr for all created messages and fields to
   * weak reference (in case this DataType goes out of scope).
   * We could possibly implement using shared_from_this(), but I can
   * see possible cases where we just make the object non-shared. */
  std::shared_ptr<GribPointerHolder> myValidPtr;
};
}
