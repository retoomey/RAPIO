#pragma once
#include <RAPIO.h>
#include <rPolarAlgorithm.h>

namespace rapio {

class Obj_SingleThresholdDriver : public rapio::RAPIOAlgorithm {
public:
  Obj_SingleThresholdDriver() : RAPIOAlgorithm("Obj_SingleThresholdDriver") {};

  virtual void declareOptions(rapio::RAPIOOptions& o) override;
  virtual void processOptions(rapio::RAPIOOptions& o) override;
  virtual void processNewData(rapio::RAPIOData& d) override;

private:
  float myThreshold = 50.0f;
};

} // namespace rapio
