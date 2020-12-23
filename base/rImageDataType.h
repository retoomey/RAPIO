#pragma once

#include <rDataType.h>
#include <rDataGrid.h>

namespace rapio {
/** DataType for holding Grib data
 * Interface for implementations of grib to
 * implement
 *
 * @author Robert Toomey */
class ImageDataType : public DataType {
public:
  ImageDataType()
  {
    myDataType = "ImageData";
  }
};
}
