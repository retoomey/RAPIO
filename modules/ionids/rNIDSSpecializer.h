#pragma once

#include <rBlockMessageHeader.h>
#include <rBlockProductDesc.h>
#include <rBlockProductSymbology.h>

namespace rapio {
/** Trying something new.  The generic read/write
 * virtual may not be actually needed.  Each module uses
 * Specializer independently so why force the API.
 * FIXME: Back change all the modules. */
class NIDSSpecializer : public IOSpecializer
{
public:

  /** Read a DataType from the NIDS headers */
  virtual std::shared_ptr<DataType>
  readNIDS(
    IOConfig              & config,
    BlockMessageHeader    & h,
    BlockProductDesc      & d,
    BlockProductSymbology & s,
    StreamBuffer          & z) = 0;

  /** Write DataType */
  virtual bool
  writeNIDS(
    IOConfig                  & config,
    std::shared_ptr<DataType> dt,
    StreamBuffer              & z) = 0;

  // Older methods placeholder
  // @Deprecated

  /** Read a RadialSet with given keys */
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
