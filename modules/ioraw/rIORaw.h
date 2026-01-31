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

  // READING ------------------------------------------------------------
  //

  /** Reader call back */
  virtual std::shared_ptr<DataType>
  createDataType(const std::string& params) override;

  /** Do a full read from a param list */
  static std::shared_ptr<DataType>
  readRawDataType(const URL& path);

  // WRITING ------------------------------------------------------------

  /** Encode this data type to path given format settings */
  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt,
    std::map<std::string, std::string>     & keys
  ) override;

  virtual
  ~IORaw();
};
}
