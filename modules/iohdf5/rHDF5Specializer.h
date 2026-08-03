#pragma once

namespace rapio {
/** All HDF5 specializers subclass this
 * allowing us to directly send HDF5 variables
 */
class HDF5Specializer : public IOSpecializer
{
public:

  /** Read a DataType from HDF5 */
  virtual std::shared_ptr<DataType>
  readHDF5(hid_t hdf5id,
    IOConfig     & keys) = 0;

  /** Write DataType to HDF5 */
  virtual bool
  writeHDF5(hid_t             hdf5id,
    std::shared_ptr<DataType> dt,
    IOConfig                  & keys){ return false; }

  // Unused.

  /** Read a DataType with given keys */
  virtual std::shared_ptr<DataType>
  read(IOConfig& config)
  override { return nullptr; }

  /** Write DataType from given ncid */
  virtual bool
  write(
    std::shared_ptr<DataType> dt,
    IOConfig                  & keys)
  override { return false; }
};
}
