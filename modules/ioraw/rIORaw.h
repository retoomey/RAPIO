#pragma once

#include "rIODataType.h"

// #include <iomanip>

namespace rapio {
/**
 * Read MRMS Merger Stage 1 data files
 * Adding this for some experiments pushing to cloud
 *
 * @author Robert Toomey
 */
class IORaw : public IODataType {
public:

  /** Help for ioimage module */
  virtual std::string
  getHelpString(const std::string& key) override;

  // Registering of classes ---------------------------------------------
  virtual void
  initialize() override;

  /** Reader call back */
  virtual std::shared_ptr<DataType>
  createDataType(IOConfig& params) override;

  /** Encode this data type to path given format settings */
  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt,
    IOConfig                               & config
  ) override;

  /** Destroy a IORaw */
  virtual
  ~IORaw();
};
}
