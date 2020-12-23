#pragma once

#include <rDataType.h>
#include <rDataGrid.h>
#include <rTime.h>
#include <rLLH.h>
#include <rIOImage.h>
#include <rImageDataType.h>

#include <memory>

namespace rapio {
/** DataType for holding image data
 * @author Robert Toomey */
class ImageDataTypeImp : public ImageDataType {
public:
  ImageDataTypeImp(const std::vector<char>& buf) : myBuf(buf)
  {
    myDataType = "ImageData";
  }

private:
  /** Store anything we have here */
  std::vector<char> myBuf;
};
}
