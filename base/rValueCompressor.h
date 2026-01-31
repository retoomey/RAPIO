#pragma once

#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

namespace rapio {
/** (AI) Create a class that can convert values into compressed bytes using the
 * smallest allowed range and precision settings.  This gives up storage and
 * precision to gain space.  For example, a double is 8 bytes but you can
 * store min = 0, max = 255, precision 0 into a single byte, saving 7 bytes.
 *
 * This helps massively with large data sets where you know the expected range
 * of values and you are willing to have a given number of decimal places.
 *
 * For example, we can store -200 to 200 with a precision of 2.  This allows
 * numbers such as "-100.43" and "123.45" but all other values are rounded.
 *
 * FIXME: Might make this class template double/float and others just to see
 * speed/ram differences.  Right now limited to double
 *
 * @author Robert Toomey
 * @ingroup rapio_utility
 * @brief Compresses values using a scale/range.
 */
class ValueCompressor {
public:
  /** Create ValueCompressor */
  ValueCompressor(double minVal, double maxVal, int precision)
    : myMinVal(minVal), myMaxVal(maxVal), myMultiplier(std::pow(10, precision))
  {
    myBytesNeeded = calculateBytesNeeded(myMinVal, myMaxVal, myMultiplier);
  }

  /** Calculate the bytes needed for given settings */
  static int
  calculateBytesNeeded(double minVal, double maxVal, int precision);

  /** Get the max value we store */
  double
  getMaxValue() const
  {
    return myMaxVal;
  }

  /** Get the min value we store */
  double
  getMinValue() const
  {
    return myMinVal;
  }

  /** Get the multiplier we're using */
  int
  getMultiplier() const
  {
    return myMultiplier;
  }

  /** Get the required bytes used */
  int
  getBytesNeeded() const
  {
    return myBytesNeeded;
  }

  /** Convert a single value into bytes.  More efficient to use the group method below,
   * but this is convenient for single values. */
  std::vector<unsigned char>
  compress(double value) const
  {
    std::vector<double> values = { value };

    return compressVector(values);
  }

  /** Convert a vector of double values into a compressed byte vector using
   * the min, max and precision settings. */
  std::vector<unsigned char>
  compressVector(const std::vector<double>& values) const
  {
    std::vector<unsigned char> bytes;

    bytes.reserve(values.size() * myBytesNeeded);

    // Make minVal an int with correct digits and cut off the rest past precision.
    const int mini = myMinVal * myMultiplier; //

    for (const double& value : values) {
      // FIXME:
      // Handle missing or data special value by reserving certain values like the max or min
      //
      int normalized = (value * myMultiplier) - mini;
      for (int i = 0; i < myBytesNeeded; ++i) {
        // Little endian here.
        bytes.push_back(static_cast<unsigned char>(normalized)); // clips off lowest byte
        normalized = normalized >> 8;                            // then move high bits lower
      }
    }

    return bytes;
  }

  /** Convert a compressed vector to a single value */
  double
  uncompress(std::vector<unsigned char>& bytes) const
  {
    std::vector<double> stuff = uncompressVector(bytes);

    return stuff[0];
  }

  /** Convert a compressed byte vector back to double using the min, max and precision settings */
  std::vector<double>
  uncompressVector(const std::vector<unsigned char>& bytes) const
  {
    std::vector<double> values;

    values.reserve(bytes.size() / myBytesNeeded);

    const int mini = myMinVal * myMultiplier;

    for (std::size_t i = 0; i < bytes.size(); i += myBytesNeeded) {
      int normalized = 0;
      for (int j = myBytesNeeded - 1; j >= 0; --j) {
        normalized  = (normalized << 8);
        normalized |= bytes[i + j];
      }
      normalized += mini;
      double doubleValue = static_cast<double>(normalized) / myMultiplier;
      // FIXME:
      // Handle missing value by reserving the max or min
      //
      values.push_back(doubleValue);
    }

    return values;
  }

protected:
  /** Minimum value we will represent */
  double myMinVal;

  /** Maximum value we will represent */
  double myMaxVal;

  /** Power of 10 of precision */
  int myMultiplier;

  /** Number of bytes required per true value */
  int myBytesNeeded;
};
}
