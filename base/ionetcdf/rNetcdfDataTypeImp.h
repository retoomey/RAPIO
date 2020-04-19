#pragma once

#include <rDataGrid.h>
#include <rIONetcdf.h>

#include <rTime.h>
#include <rLLH.h>

#include <memory>
#include <string>

namespace rapio {
/** DataType for the data of a Netcdf file before any possible
 * delegation.  Note, on delegation the delegated data type
 * might not be a direct subclass.
 *
 * @author Robert Toomey */
class NetcdfDataTypeImp : public DataGrid {
public:
  NetcdfDataTypeImp(const std::vector<char>& buf) : myBuf(buf) // copy or move?
  {
    myDataType = "NetcdfData";
  }

  /** Return Location that corresponds to this DataType */
  virtual LLH
  getLocation() const override;

  /** Return Time that corresponds to this DataType */
  virtual Time
  getTime() const override;

private:
  /** Store the buffer of data (copy wraps around shared_ptr) */
  std::vector<char> myBuf;
};
}