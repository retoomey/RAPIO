#pragma once

#include <rUtility.h>

#include <string>
#include <cmath> // for fabs()
#include <stdio.h>

namespace rapio {
/**
 * Convenience math operations
 */
class Arith : public Utility
{
public:

  /** Compress a float percent to a char 0 to 100 with range clipping.
   * values are rounded down.  This is used for compression in
   * certain situations where we can use a 8 bit char instead of the
   * longer float */
  static inline unsigned char
  floatPercentToChar(float v)
  {
    // Ok pin the fraction to 0-100 percent
    int result = int(0.5 + v * 100);

    if (v < 0) {
      v = 0;
    } else if (v > 100) {
      v = 100;
    }
    return v;
  }

  /** Round the given number to the nearest integer. */
  static inline int
  roundOff(double n)
  {
    return (n > 0 ? int(n + 0.5) : int(n - 0.5));
  }

  /** Round the given number to the nearest increment.
   *  For example, 3.14159 to the nearest 0.1 is 3.1. */
  static inline double
  roundOff(double n, double incr)
  {
    return (incr * roundOff(n / incr));
  }

  /** Compare doubles to within a certain accuracy.  */
  static inline bool
  feq(double n1, double n2, double accuracy = 1.0E-05)
  {
    return (fabs(n1 - n2) < accuracy);
  }

  /**
   * Return a number less than, equal to, or greater than zero
   * if a is less than, equal to, or greater than b.
   */
  static inline int
  compare(double a, float b, double accuracy = 1.0E-05)
  {
    if (feq(a, b, accuracy)) { return (0); }

    return (a < b ? -1 : 1);
  }

  template <class T>
  static inline std::string
  str(const T& x, const char * fmt)
  {
    char buf[256];

    snprintf(buf, sizeof(buf), fmt, x);
    return (buf);
  }

  static inline std::string
  str(int x)
  {
    return (str(x, "%d"));
  }

  static inline std::string
  str(long x)
  {
    return (str(x, "%l"));
  }

  static inline std::string
  str(unsigned long x)
  {
    return (str(x, "%lu"));
  }

  static inline std::string
  str(double x)
  {
    return (str(x, "%f"));
  }
};
}
