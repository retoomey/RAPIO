#pragma once

#include "rIODataType.h"
#include "rDataGrid.h"
#include "rIO.h"

#include <iomanip>

namespace rapio {
/**
 * Python
 *
 * @author Robert Toomey
 */
class IOPython : public IODataType {
public:

  // Registering of classes ---------------------------------------------
  virtual void
  initialize() override;

  // READING ------------------------------------------------------------
  //

  /** Reader call back */
  virtual std::shared_ptr<DataType>
  createDataType(const std::string& params) override;

  // WRITING ------------------------------------------------------------

  /** Run python */
  std::vector<std::string> runDataProcess(const std::string& command, 
    const std::string& filename, std::shared_ptr<DataGrid> datagrid);

  /** Encode this data type to path given format settings */
  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt,
    const std::string                      & params,
    std::shared_ptr<PTreeNode>             dfs,
    bool                                   directFile,
    // Output for notifiers
    std::vector<Record>                    & records
  ) override;

  virtual
  ~IOPython();
};
}
