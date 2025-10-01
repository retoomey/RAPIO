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
 * Toomey.  Sept 2025
 * Vertical Column Coverage (VCC) Metric
 * (My attempt at a metric)
 *
 * This algorithm calculates the percentage of vertical coverage for a radar beam
 * at a given range. It serves as a weighting factor to evaluate the completeness
 * of a vertical column scan.
 *
 * The metric is designed to account for beam spreading with increasing range.
 * As the beam spreads, the percentage of the column it covers increases.
 * A higher percentage of coverage indicates a more complete scan of the vertical
 * column.
 *
 * This metric is used in conjunction with other factors, such as an inverse
 * range weighting, to produce a final metric that can be used for weighting N radars.
 *
 * The calculation is based on the vertical distance covered by the beam relative
 * to a predefined cut-off height. The metric is expressed in a linear dimension,
 * not an angular one, which ensures its independence from range and allows
 * for direct comparison.
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

  /** Vertical Column Coverage */
  void
  VerticalColumnCoverage(const Time& useTime, float useElevDegs, const std::string& useSubtype);

  /** The traditional EET beam top algorithm */
  void
  Traditional(const Time& useTime, float useElevDegs, const std::string& useSubtype);

  /** The Interpolated beam top algorithm from Lak's paper */
  void
  Interpolated(const Time& useTime, float useElevDegs, const std::string& useSubtype);
};
}
