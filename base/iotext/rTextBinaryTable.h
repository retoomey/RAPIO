#pragma once

#include <rIOText.h>
#include "rBinaryTable.h"
#include "rURL.h"

namespace rapio {
/** Handles the read/write of DataGrid DataType from a text file.
 * @author Robert Toomey
 */
class TextBinaryTable : public IOSpecializer {
public:

  /** Read DataType with given keys */
  virtual std::shared_ptr<DataType>
  read(
    std::map<std::string, std::string>& keys,
    std::shared_ptr<DataType>         dt)
  override;

  /** Write DataType with given keys */
  virtual bool
  write(
    std::shared_ptr<DataType>         dt,
    std::map<std::string, std::string>& keys)
  override;

  /** Destroy the writer */
  virtual
  ~TextBinaryTable();

  /** Initial introduction of TextBinaryTable specializer to IOText */
  static void
  introduceSelf(IOText * owner);
}
;
}
