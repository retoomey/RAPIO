#pragma once

/** RAPIO API */
#include <RAPIO.h>

namespace rapio {
/*
 *  Generic copy of DataType to another location.
 *  Since this handles -o options, this can also be used
 *  as a default file format converter.
 *
 * @author Robert Toomey
 **/
class Copy : public rapio::RAPIOAlgorithm {
public:

  /** Create an example simple algorithm */
  Copy(){ };

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

  /** A delay before processing a file in seconds, < zero if no delay.  Useful
   * for certain testing and other things. */
  float myBeforeDelaySeconds;

  /** A delay after processing a file in seconds, < zero if no delay.  Useful
   * for certain testing and other things. */
  float myAfterDelaySeconds;

  /** When writing, update the DataType time to current time */
  bool myUpdateTime;
};
}
