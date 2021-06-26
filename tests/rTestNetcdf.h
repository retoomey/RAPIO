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

  /** Total files we processed */
  size_t totalCount;

  /** Total time for each file type */
  TimeDuration totalTimes[11];
};
}
