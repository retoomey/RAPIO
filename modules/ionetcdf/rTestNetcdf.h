#pragma once

/** RAPIO API */
#include <RAPIO.h>

namespace rapio {
class RAPIONetcdfTestAlg : public rapio::RAPIOAlgorithm {
public:

  /** Create netcdf testing algorithm */
  RAPIONetcdfTestAlg(){ };

  /** Declare all algorithm options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(rapio::RAPIOOptions& o) override;

  /** Process a new record/datatype */
  virtual void
  processNewData(rapio::RAPIOData& d) override;

  /** Summeries of process times */
  ProcessTimerSum totalSums[12];

private:
  /** Max files to process before stopping */
  size_t myMaxCount;
};
}
