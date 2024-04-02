#pragma once

/** RAPIO API */
#include <RAPIO.h>

namespace rapio {
/*
 * Generic dump of DataType to text tool
 *
 * @author Robert Toomey
 **/
class Dump : public rapio::RAPIOAlgorithm {
public:

  /** Create */
  Dump(){ };

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
