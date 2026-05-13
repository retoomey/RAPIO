#pragma once

#include <rRAPIOAlgorithm.h>
#include <rProcessTimer.h>

namespace rapio {
class RAPIONetcdfTestAlg : public RAPIOAlgorithm {
public:

  /** Create netcdf testing algorithm */
  RAPIONetcdfTestAlg(){ };

  /** Declare all algorithm options */
  virtual void
  declareOptions(RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(RAPIOOptions& o) override;

  /** Process a new record/datatype */
  virtual void
  processNewData(RAPIOData& d) override;

  /** Summeries of process times */
  ProcessTimerSum totalSums[12];

private:
  /** Max files to process before stopping */
  size_t myMaxCount;
};
}
