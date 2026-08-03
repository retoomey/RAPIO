#pragma once

namespace rapio {
/** All netcdf specializers subclass this
 * allowing us to directly send netcdf variables
 */
class NetcdfSpecializer : public IOSpecializer
{
public:

  /** Read a DataType from NETCDF */
  virtual std::shared_ptr<DataType>
  readNETCDF(int ncid,
    IOConfig     & keys) = 0;

  /** Write DataType to NETCDF */
  virtual bool
  writeNETCDF(int             ncid,
    std::shared_ptr<DataType> dt,
    IOConfig                  & keys){ return false; }

  // Unused.

  /** Read a RadialSet with given keys */
  virtual std::shared_ptr<DataType>
  read(IOConfig& keys)
  override { return nullptr; }

  /** Write DataType from given ncid */
  virtual bool
  write(
    std::shared_ptr<DataType> dt,
    IOConfig                  & keys)
  override { return false; }
};
}
