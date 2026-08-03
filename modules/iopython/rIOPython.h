#pragma once

#include "rIODataType.h"
#include "rDataGrid.h"

#include <iomanip>

namespace rapio {
/**
 * Python
 *
 * @author Robert Toomey
 */
class IOPython : public IODataType {
public:

  /** Help for python module */
  virtual std::string
  getHelpString(const std::string& key) override;

  // Registering of classes ---------------------------------------------
  virtual void
  initialize() override;

  // READING ------------------------------------------------------------
  //

  /** Reader call back */
  virtual std::shared_ptr<DataType>
  createDataType(IOConfig& config) override;

  // WRITING ------------------------------------------------------------

  /** Run python */
  std::vector<std::string>
  runDataProcess(const std::string& command,
    const std::string& filename, std::shared_ptr<DataGrid> datagrid);

  /** Handle parsing the command line param.  For example
   * factory=outputfolder, or factory= script, outputfolder.
   * This turns the command line into the param map values */
  virtual void
  handleCommandParam(const std::string & command,
    IOConfig                           &outputParams) override;

  /** Encode this data type to path given format settings */
  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt,
    IOConfig                               & keys
  ) override;

  virtual
  ~IOPython();
};
}
