#pragma once
#include <rIOText.h>
#include "rTimeHeightGrid.h"

namespace rapio {

class TextTimeHeightGrid : public IOSpecializer {
public:
  virtual std::shared_ptr<DataType>
  read(IOConfig& config) override;

  virtual bool
  write(std::shared_ptr<DataType> dt, IOConfig& keys) override;

  virtual ~TextTimeHeightGrid();

  static void
  introduceSelf(IOText * owner);
};

} // namespace rapio
