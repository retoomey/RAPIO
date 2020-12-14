#pragma once

#include <rDataGrid.h>
#include <rTime.h>
#include <rLLH.h>
#include <rIONetcdf.h>
#include <rNetcdfDataType.h>

#include <memory>
#include <string>

namespace rapio {
/** DataType for the data of a Netcdf file before any possible
 * delegation.  Note, on delegation the delegated data type
 * might not be a direct subclass.
 *
 * @author Robert Toomey */
class NetcdfDataTypeImp : public NetcdfDataType {
public:
  NetcdfDataTypeImp(const std::vector<char>& buf) : myBuf(buf) // copy or move?
  {
    myDataType = "NetcdfData";
  }

private:
  /** Store the buffer of data (copy wraps around shared_ptr) */
  std::vector<char> myBuf;
};
}
