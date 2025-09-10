#pragma once

/** RAPIO API */
#include <RAPIO.h>

namespace rapio {
/*
 *  A polar maximum value calculator.
 *  It calculates max value in the polar column.
 *
 * @author Robert Toomey
 **/
class PolarVMax : public rapio::PolarAlgorithm {
public:

  /** Create an example simple algorithm */
  PolarVMax(){ };

  /** Declare all algorithm options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;

  /** Process the virtual volume. */
  virtual void
  processVolume(const Time& outTime, float useElevDegs, const std::string& useSubtype) override;
};
}
