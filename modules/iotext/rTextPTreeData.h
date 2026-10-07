#pragma once
#include <rIOText.h>
#include <rPTreeData.h>

namespace rapio {
class TextPTreeData : public IOSpecializer {
public:
  virtual std::shared_ptr<DataType>
  read(IOConfig& config) override;
  virtual bool
  write(std::shared_ptr<DataType> dt, IOConfig& keys) override;
  virtual
  ~TextPTreeData() = default;

  static void
  introduceSelf(IOText * owner);
};
}
