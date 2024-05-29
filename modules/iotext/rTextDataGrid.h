#pragma once

#include <rIOText.h>
#include "rDataGrid.h"
#include "rURL.h"

namespace rapio {
/** Handles the read/write of DataGrid DataType from a text file.
 * The plan for this module is mostly dumping data formats to
 * a human readable format, say in the form of ncdump.
 *
 * @author Robert Toomey
 */
class TextDataGrid : public IOSpecializer {
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

  /** Destroy a text data grid */
  virtual
  ~TextDataGrid();

  /** Initial introduction of TextDataGrid specializer to IOText */
  static void
  introduceSelf(IOText * owner);
}
;
}
