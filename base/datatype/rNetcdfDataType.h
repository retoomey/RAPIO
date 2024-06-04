#pragma once

#include <rDataGrid.h>

#include <rTime.h>
#include <rLLH.h>

namespace rapio {
/** DataType for the data of a Netcdf file before any possible
 * Interface for implementations of netcdf to
 * implement.  Currently I'm forcing you to use DataGrid
 *
 * @author Robert Toomey */
class NetcdfDataType : public DataGrid {
public:
  NetcdfDataType()
  {
    myDataType = "NetcdfData";
  }
};
}
