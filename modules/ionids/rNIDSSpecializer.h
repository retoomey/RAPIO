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
    std::map<std::string, std::string>& keys,
    BlockMessageHeader                & h,
    BlockProductDesc                  & d,
    BlockProductSymbology             & s,
    StreamBuffer                      & z) = 0;

  /** Write DataType */
  virtual bool
  writeNIDS(
    std::map<std::string, std::string>& keys,
    std::shared_ptr<DataType>         dt,
    StreamBuffer                      & z) = 0;

  // Older methods placeholder
  // @Deprecated

  /** Read a RadialSet with given keys */
  virtual std::shared_ptr<DataType>
  read(
    std::map<std::string, std::string>& keys,
    std::shared_ptr<DataType>         dt)
  override { return nullptr; }

  /** Write DataType from given ncid */
  virtual bool
  write(
    std::shared_ptr<DataType>         dt,
    std::map<std::string, std::string>& keys)
  override { return false; }
};
}
