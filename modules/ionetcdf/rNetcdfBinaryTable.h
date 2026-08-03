#pragma once

#include <rIONetcdf.h>
#include <rNetcdfSpecializer.h>

class NcVar;

namespace rapio {
class BinaryTable;

/** Handles the construction of BinaryTable object from the netcdf data,
 *  @see IONetcdf
 */
class NetcdfBinaryTable : public NetcdfSpecializer {
public:

  /** Destroy this NetcdfBinaryTable */
  virtual
  ~NetcdfBinaryTable();

  /** Initial introduction of BinaryTable specializer to IONetcdf */
  static void
  introduceSelf(IONetcdf * owner);

  /** Read a BinaryTable from NETCDF */
  virtual std::shared_ptr<DataType>
  readNETCDF(int ncid,
    IOConfig     & keys) override;

  /** Write a DataType to NETCDF */
  virtual bool
  writeNETCDF(int             ncid,
    std::shared_ptr<DataType> dt,
    IOConfig                  & keys) override;
};
}
