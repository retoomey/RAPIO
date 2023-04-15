#pragma once

/** RAPIO API */
#include <RAPIO.h>

namespace rapio {
/*
 *  Watches any -i RAPIO ingest and prints information
 *
 * @author Robert Toomey
 **/
class Watch : public rapio::RAPIOAlgorithm {
public:

  /** Create an example simple algorithm */
  Watch(){ };

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
}
