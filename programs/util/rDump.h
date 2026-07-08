#pragma once

#include <rRAPIOAlgorithm.h>

namespace rapio {
/*
 * Generic dump of DataType to text tool
 *
 * @author Robert Toomey
 **/
class Dump : public RAPIOAlgorithm {
public:

  /** Create */
  Dump(){ };

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
