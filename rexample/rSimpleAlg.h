#pragma once

/** RAPIO API */
#include <RAPIO.h>

namespace wdssii { // or whatever you want
/** Create your algorithm as a subclass of RAPIOAlgorithm */
class W2SimpleAlg : public rapio::RAPIOAlgorithm {
public:

  /** Create an accumulator */
  W2SimpleAlg(){ };

  /** Declare all algorithm options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o);

  /** Process all algorithm options */
  virtual void
  processOptions(rapio::RAPIOOptions& o);

  /** Process a new record/datatype.  See the .cc for RAPIOData info */
  virtual void
  processNewData(rapio::RAPIOData& d);

protected:

  /** My test example optional string parameter */
  std::string myTest;

  /** My test example optional boolean parameter */
  bool myX;

  /** My test example REQUIRED stringparameter */
  std::string myZ;
};
}
