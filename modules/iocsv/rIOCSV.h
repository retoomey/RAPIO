#pragma once
#include <rIODataType.h>
#include <memory>
#include <string>

namespace rapio {

/** General CSV read/write module
 * @author Robert Toomey
 */
class IOCSV : public IODataType {
public:
  virtual ~IOCSV() = default;

  virtual std::string
  getHelpString(const std::string& key) override;

  virtual void
  initialize() override;

  virtual std::shared_ptr<DataType>
  createDataType(IOConfig& config) override;

  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt, IOConfig& keys) override;
};

} // namespace rapio
