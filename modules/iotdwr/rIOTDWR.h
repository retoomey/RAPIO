/**
 * @file rIOTDWR.h
 * @brief Dynamic RAPIO IODataType plugin for Terminal Doppler Weather Radar (TDWR).
 */

#pragma once

#include <rIODataType.h>

namespace rapio {
/**
 * @class IOTDWR
 * @brief Main I/O DataType module handler for TDWR binary files and streams.
 */
class IOTDWR : public IODataType {
public:
  IOTDWR();
  virtual
  ~IOTDWR();

  virtual std::string
  getHelpString(const std::string& key) override;
  virtual void
  initialize() override;

  virtual std::shared_ptr<DataType>
  createDataType(IOConfig& config) override;
  virtual std::shared_ptr<DataType>
  createDataTypeFromBuffer(std::vector<char>& buffer) override;
};
} // namespace rapio
