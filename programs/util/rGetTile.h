#pragma once

/** RAPIO API */
#include <RAPIO.h>

namespace rapio {
class RAPIOTileAlg : public rapio::RAPIOAlgorithm {
public:

  /** Create tile algorithm */
  RAPIOTileAlg(){ };

  /** Declare all algorithm options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(rapio::RAPIOOptions& o) override;

  /** Process a new record/datatype */
  virtual void
  processNewData(rapio::RAPIOData& d) override;

protected:

  /** Override output params for image output (global) */
  std::map<std::string, std::string> myOverride;
};
}
