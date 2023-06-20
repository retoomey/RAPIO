#pragma once

/** RAPIO API */
#include <RAPIO.h>

class GribExampleAlg : public rapio::RAPIOAlgorithm {
public:

  /** Create an example simple algorithm */
  GribExampleAlg(){ };

  // The basic API messages from the system

  /** Declare all algorithm options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(rapio::RAPIOOptions& o) override;

  /** Process a new record/datatype.  See the .cc for RAPIOData info */
  virtual void
  processNewData(rapio::RAPIOData& d) override;

protected:
};
