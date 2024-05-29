#pragma once

#include <rDataType.h>
#include <rDataGrid.h>
#include <rTime.h>
#include <rLLH.h>
#include <rIOGDAL.h>
#include <rGDALDataType.h>

#include <memory>

namespace rapio {
/** DataType for holding image data
 * @author Robert Toomey */
class GDALDataTypeImp : public GDALDataType {
public:

  /** Create a GDAL wrapper */
  GDALDataTypeImp()
  {
    myDataType = "GDALData";
  }

  /** First attempt read a shapefile test.  Since we hide the gdal dependency from
   * RAPIO core, we'd need to expand an interface in GDALDataType to 'do' anything.
   * FIXME: Expand interface, thinking at least some shapefile support would be nice. */
  bool
  readGDALDataset(const std::string& key);

private:
};
}
