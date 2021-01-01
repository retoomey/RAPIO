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
  GDALDataTypeImp(const std::vector<char>& buf) : myBuf(buf)
  {
    myDataType = "GDALData";
  }

private:
  /** Store anything we have here */
  std::vector<char> myBuf;
};
}
