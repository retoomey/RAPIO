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

  // FIXME: not liking this forced at this level...
  //
  /** Return Location that corresponds to this DataType */
  virtual LLH
  getLocation() const override;

  /** Return Time that corresponds to this DataType */
  virtual Time
  getTime() const override;

  /** Print the catalog for thie GribDataType. */
  virtual void
  printCatalog() = 0;

  /** One way to get 2D data, using key and level string like our HMET library */
  virtual std::shared_ptr<RAPIO_2DF>
  getFloat2D(const std::string& key, const std::string& levelstr, size_t& x, size_t& y) = 0;
};
}
