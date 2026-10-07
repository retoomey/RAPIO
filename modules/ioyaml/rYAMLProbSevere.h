#pragma once
#include <rIOYAML.h>
#include <rDataTable.h> // For PTreeDataSpecializer
#include "rProbSevere.h"

namespace rapio {
class YAMLProbSevere : public PTreeDataSpecializer {
public:
  static void
  introduceSelf(IOYAML * owner);

  virtual bool
  canHandle(std::shared_ptr<PTreeData> tree) override;

  virtual std::shared_ptr<DataType>
  downcastPTreeDataType(IOConfig& config, std::shared_ptr<DataType> in) override;
};
} // namespace rapio
