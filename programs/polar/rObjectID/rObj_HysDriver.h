#pragma once
#include <RAPIO.h>

namespace rapio {

class Obj_HysDriver : public rapio::RAPIOAlgorithm {
public:
  Obj_HysDriver() : RAPIOAlgorithm("Obj_HysDriver") {};

  virtual void declareOptions(rapio::RAPIOOptions& o) override;
  virtual void processOptions(rapio::RAPIOOptions& o) override;
  virtual void processNewData(rapio::RAPIOData& d) override;

private:
  std::vector<float> myThresholds;
  std::vector<size_t> myMinSizes;
};

} // namespace rapio
