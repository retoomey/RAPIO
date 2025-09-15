#pragma once

/** RAPIO API */
#include <RAPIO.h>

namespace rapio {
/*
 *  A polar Echo-Top calculator.
 *
 * @author Valliappa Lakshmanan (Lak)
 * @author Robert Toomey
 *
 * Paper: An Improved Method for Estimating Radar Echo-Top Height
 *
 * Porting Lak's work with thie paper so we can experiment with
 * building upon it.  So far the following notes:
 * 1.  w2echotop didn't seem to add the radar height into the math,
 *     feel like we need this to normalize heights for any merge/fusion
 *     There appears to be a bug in the location method called.
 *
 **/
class EchoTop : public rapio::PolarAlgorithm {
public:

  /** Create an example simple algorithm */
  EchoTop(){ };

  /** Declare all algorithm options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;

  /** Process the virtual volume. */
  virtual void
  processVolume(const Time& outTime, float useElevDegs, const std::string& useSubtype) override;

  /** The traditional EET beam top algorithm */
  void
  Traditional(const Time& useTime, float useElevDegs, const std::string& useSubtype);

  /** The Interpolated beam top algorithm from Lak's paper */
  void
  Interpolated(const Time& useTime, float useElevDegs, const std::string& useSubtype);
};
}
