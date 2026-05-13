#pragma once

#include <rRAPIOAlgorithm.h>

namespace rapio {
/*
 *  Watches any -i RAPIO ingest and prints information
 *
 * @author Robert Toomey
 **/
class Watch : public RAPIOAlgorithm {
public:

  /** Create an example simple algorithm */
  Watch(){ };

  /** Declare all algorithm options */
  virtual void
  declareOptions(RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(RAPIOOptions& o) override;

  /** Process a new record/datatype.  See the .cc for RAPIOData info */
  virtual void
  processNewData(RAPIOData& d) override;

protected:
};
}
