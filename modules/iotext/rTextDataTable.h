#pragma once
#include <rIOText.h>
#include "rDataTable.h"

namespace rapio {
class TextDataTable : public IOSpecializer {
public:
  virtual std::shared_ptr<DataType> read(IOConfig& config) override;
  virtual bool write(std::shared_ptr<DataType> dt, IOConfig& keys) override;
  virtual ~TextDataTable() = default;
  static void introduceSelf(IOText * owner);
};
}
