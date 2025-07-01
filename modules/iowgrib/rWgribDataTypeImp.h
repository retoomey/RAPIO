#pragma once

#include <rGribDataType.h>
#include <rCatalogCallback.h>

namespace rapio {
/** DataType for holding Grib data
 * Implement the GribDataType interface from RAPIO
 * to provide Grib2 support.
 * @author Robert Toomey */
class WgribDataTypeImp : public GribDataType {
public:
  /** Construct a grib2 implementation */
  WgribDataTypeImp(const URL& url);

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

  /** Get a projected LatLonGrid from the grib data */
  std::shared_ptr<LatLonGrid>
  getLatLonGrid(const std::string& key, const std::string& levelstr);

protected:

  /** Match a single field in a grib2 file */
  std::shared_ptr<CatalogCallback>
  haveSingleMatch(const std::string& match);

private:

  /** Store the URL passed to use for filename */
  URL myURL;
};
}
