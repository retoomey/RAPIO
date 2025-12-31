#pragma once

#include "rUtility.h"
#include "rTime.h"

#include <array>
#include <vector>

namespace rapio {
/** Various utilities for generating color codes and mrms values. */
class NIDSUtil : public Utility {
public:

  // ------------------------------------------------
  // Time conversion

  /** Given NIDS date and time values, convert to Time object */
  static Time
  getTimeFromNIDSTime(short volScanDate, int volScanStartTime);

  /** Given a Time, convert to the NIDS data and time values */
  static void
  getNIDSTimeFromTime(const Time& t, short& volScanDate, int& volScanStartTime);

  // ------------------------------------------------
  // Convert raw character data into color codes
  //

  /** Convert raw using run-length encoded colors */
  static void
  getRLEColors(const std::vector<char> & src, std::vector<int> & data);

  /** Convert raw using colors */
  static void
  getColors(const std::vector<char> & src,
    std::vector<int>                & data);

  /** Convert raw using generic colors */
  static void
  getGenericColors(const std::vector<char> & src,
    std::vector<int>                       & data);

  // ------------------------------------------------
  // Convert color codes into actual MRMS values
  // Either direct encoded thresholds from nids are used,
  // or decoded values.

  // Direct encoded value methods

  /** Method D1 uses decoded thresholds (original product 164) */
  static void
  colorToValueD1(
    const std::vector<int>   & colors,
    const std::vector<float> & thresholds,
    std::vector<float>       & values);

  /** Method D2 uses decoded thresholds (original product 165, 176, 177) */
  static void
  colorToValueD2(
    const std::vector<int>   & colors,
    const std::vector<float> & thresholds,
    std::vector<float>       & values);

  /** Method D3 uses decoded thresholds
   * (original product 94, 99, 153, 154, 155, 159, 161, 163) */
  static void
  colorToValueD3(
    const std::vector<int>   & colors,
    const std::vector<float> & thresholds,
    std::vector<float>       & values);

  /** Method D3 reverse */
  static void
  valueToColorD3(
    const std::vector<float>& thresholds,
    const float *           values, // to avoid copying things
    const size_t            size,
    std::vector<int>        & colors);

  /** Method D4 uses decoded thresholds (default product ) */
  static void
  colorToValueD4(
    const std::vector<int>   & colors,
    const std::vector<float> & thresholds,
    std::vector<float>       & values);

  /** Method E1 uses default encoded thresholds (original product 134) */
  static void
  colorToValueE1(const std::vector<int> & color,
    std::array<short, 16>               & encoded,
    std::vector<float>                  & values);

  /** Method E2 uses default encoded thresholds (original product 135) */
  static void
  colorToValueE2(const std::vector<int> & color,
    std::array<short, 16>               & encoded,
    std::vector<float>                  & values);

  /** Use encoded/default thresholds to create RAPIO values.
   * Method E2 uses default thresholds (original product 176) */
  static void
  colorToValueE3(const std::vector<int> & color,
    std::array<short, 16>               & encoded,
    std::vector<float>                  & values);
};
}
