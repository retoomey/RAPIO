#pragma once

#include <rDataType.h>
#include <rDataGrid.h>

namespace rapio {
/** DataType for holding GDAL data
 * Interface for implementations of GDAL to
 * implement
 *
 * @author Robert Toomey */
class GDALDataType : public DataType {
public:
  GDALDataType()
  {
    myDataType = "GDALData";
  }
};
}
