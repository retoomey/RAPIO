#pragma once

#include <rData.h>

#include <cmath> // for fabs
#include <ctime> // for time_t

namespace rapio {
/**
 * Sentinel values are values that represent some condition.
 * You can use sentinel values only for setting/getting values
 * to floating point numbers, and to do equal/not-equal operations.
 * This class ensures that sentinels
 * are used such that round-off errors are handled.
 *
 * <pre>
 * SentinelDouble MissingTemperature( -999, 0.1 );
 * double v = getTemperature();
 * if ( v != MissingTemperature ){
 *   return ( 1.4*v + 32 );
 * } else
 *   return 72;
 * </pre>
 *
 * @see Constants::MissingData
 */
class SentinelDouble : public Data {
  int value;
  double tolerance;

public:

  /** the input value has to be an integer value. This class
   *  allows comparisions with doubles in a tolerant way. */
  SentinelDouble(int value_in, double tolerance_in)
    :
    value(value_in), tolerance(tolerance_in)
  { }

  /** For STL use only -- variables remain uninitialized! */
  SentinelDouble()
  { }

  /** WARNING! this is meant for use simply to set values
   *  i.e.
   *  double m = Constants::MissingData
   *  Do not use m for future comparisions. Use the value
   *  wrapped within the class (Constants::MissingData)
   *  for all comparisions.
   */
  operator const int& () const {
    return (value);
  }

  /** overload comparision for char, short, int, float and double */
  template <class T>
  friend bool
  operator == (const SentinelDouble& a, T b)
  {
    return (::fabs(a.value - b) < a.tolerance);
  }

  template <class T>
  friend bool
  operator == (T b, const SentinelDouble& a)
  {
    return (::fabs(a.value - b) < a.tolerance);
  }

  template <class T>
  friend bool
  operator != (const SentinelDouble& a, T b)
  {
    return (::fabs(a.value - b) >= a.tolerance);
  }

  template <class T>
  friend bool
  operator != (T b, const SentinelDouble& a)
  {
    return (::fabs(a.value - b) >= a.tolerance);
  }
};
}
