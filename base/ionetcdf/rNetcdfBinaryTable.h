#pragma once

#include <rIONetcdf.h>

class NcVar;

namespace rapio {
class BinaryTable;

/** Handles the construction of BinaryTable object from the netcdf data,
 *  @see IONetcdf
 */
class NetcdfBinaryTable : public IOSpecializer {
public:

  /** Write BinaryTable DataType */
  virtual bool
  write(
    std::shared_ptr<DataType> dt,
    std::map<std::string, std::string>& keys)
  override;

  /** Read a BinaryTable DataType */
  virtual std::shared_ptr<DataType>
  read(
    std::map<std::string, std::string>& keys,
    std::shared_ptr<DataType> dt)
  override;

  /** Destroy this NetcdfBinaryTable */
  virtual
  ~NetcdfBinaryTable();

  /** Initial introduction of BinaryTable specializer to IONetcdf */
  static void
  introduceSelf(IONetcdf * owner);
};
}
