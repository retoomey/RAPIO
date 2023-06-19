#pragma once

#include <rDataType.h>
#include <rDataGrid.h>

namespace rapio {
/** DataType for holding Grib data
 * Interface for implementations of grib to
 * implement
 *
 * @author Robert Toomey */
class GribDataType : public DataType {
public:
  GribDataType()
  {
    myDataType = "GribData";
  }

  /** Print the catalog for thie GribDataType. */
  virtual void
  printCatalog() = 0;

  /** One way to get 2D data, using key and level string like our HMET library */
  virtual std::shared_ptr<Array<float, 2> >
  getFloat2D(const std::string& key, const std::string& levelstr, size_t& x, size_t& y) = 0;

  /** Get a 3D array from grib data */
  virtual std::shared_ptr<Array<float, 3> >
  getFloat3D(const std::string& key, std::vector<std::string> zLevelsVec, size_t&x, size_t&y, size_t&z) = 0;
};
}
