#pragma once
#include <rIODataType.h>
#include <rPTreeData.h>
#include <memory>
#include <vector>

namespace rapio {
class IOYAML : public IODataType {
public:
  virtual std::string
  getHelpString(const std::string& key) override;
  virtual void
  initialize() override;
  virtual std::shared_ptr<DataType>
  createDataTypeFromBuffer(std::vector<char>& buffer) override;
  virtual std::shared_ptr<DataType>
  createDataType(IOConfig& config) override;

  virtual bool
  encodeDataType(std::shared_ptr<DataType> dt, IOConfig& config) override;
  virtual size_t
  encodeDataTypeBuffer(std::shared_ptr<DataType> dt, std::vector<char>& buffer, IOConfig& config) override;

  virtual
  ~IOYAML() = default;

protected:
  static std::shared_ptr<PTreeData>
  readPTreeDataBuffer(std::vector<char>& buffer);
};
}
