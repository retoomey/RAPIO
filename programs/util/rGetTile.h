#pragma once

#include <rRAPIOAlgorithm.h>

namespace rapio {
class MultiDataType;

class RAPIOTileAlg : public RAPIOAlgorithm {
public:

  /** Create tile algorithm */
  RAPIOTileAlg(){ };

  /** Declare all algorithm options */
  virtual void
  declareOptions(RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(RAPIOOptions& o) override;

  /** Process a new record/datatype */
  virtual void
  processNewData(RAPIOData& d) override;

protected:

  /** Override output params for image output (global) */
  std::map<std::string, std::string> myOverride;

  /** Manually set colormap for output */
  std::string myColorMap;

  /** Collect DataTypes to overlay each other */
  std::shared_ptr<MultiDataType> myMulti;
};
}
