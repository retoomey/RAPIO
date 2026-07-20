#pragma once
#include <RAPIO.h>

namespace rapio {

class Obj_MultiThresholdDriver : public rapio::RAPIOAlgorithm {
public:
  Obj_MultiThresholdDriver() : RAPIOAlgorithm("Obj_MutliThresholdDriver") {};

  virtual void declareOptions(rapio::RAPIOOptions& o) override;
  virtual void processOptions(rapio::RAPIOOptions& o) override;
  virtual void processNewData(rapio::RAPIOData& d) override;

private:
  std::vector<float> myThresholds;
};

} // namespace rapio
