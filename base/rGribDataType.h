#pragma once

#include <rDataType.h>
#include <rDataGrid.h>
#include <rTime.h>
#include <rLLH.h>
#include <rIOGrib.h>

#include <memory>

namespace rapio {
/** DataType for holding Grib data
 *
 * @author Robert Toomey */
class GribDataType : public DataType {
public:
  GribDataType(const std::vector<char>& buf) : myBuf(buf)
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

  /** Print the catalog for thie GribDataType.
   * FIXME: better to be able to get a catalog I think */
  void
  printCatalog();

  /** One way to get 2D data, using key and level string like our HMET library */
  std::shared_ptr<RAPIO_2DF>
  getFloat2D(const std::string& key, const std::string& levelstr);

private:
  /** Store the buffer of data (copy wraps around shared_ptr) */
  std::vector<char> myBuf;
};
}
