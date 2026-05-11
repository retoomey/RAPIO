#pragma once

#include <rDataGrid.h>
#include <rTime.h>
#include <rLLH.h>

namespace rapio {
/** DataType for holding Grib data
 * Interface for implementations of grib to
 * implement
 *
 * @author Robert Toomey */
class ImageDataType : public DataGrid {
public:
  /** Create an image DataType */
  ImageDataType()
  {
    myDataType = "ImageData";
  }

  /** Create a image with multiple channels */
  static std::shared_ptr<ImageDataType>
  Create(const std::string& name, size_t width, size_t height, size_t channels = 1)
  {
    auto img = std::make_shared<ImageDataType>();

    // Initialize DataGrid with Y, X, C dimensions
    img->init(name, "pixel", LLH(), Time::CurrentTime(),
      { height, width, channels },
      { "Y", "X", "C" });

    // Add the raw byte storage array using DataGrid's macros
    img->addByte3D(Constants::PrimaryDataName, "pixel", { 0, 1, 2 });

    return img;
  }
};
}
