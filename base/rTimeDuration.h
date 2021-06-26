#pragma once

#include <rData.h>

#include <chrono>

namespace rapio {
class Time;

/** A directed interval of time on the universal time-line,
 * cooresponds to a chrono duration
 *
 *  @see Time
 */
class TimeDuration : public Data {
  friend Time;

private:

  /** Create a TimeDuration with given milliseconds.
   * Don't allow stuff like aTimeDuration + 6 to implicit convert, it's confusing.
   * Instead, enforce: aTimeDuration + TimeDuration::Seconds(6)
   * Enforce private, most callers should use the static functions to specify units.
   */
  explicit TimeDuration(double s)
    : myMSeconds(std::chrono::milliseconds((long)(s)))
  { }

public:

  /** Create a default TimeDuration of 0 length */
  TimeDuration()
    : myMSeconds(0)
  { }

  /** Return a TimeDuration explicitly defined in milliseconds. */
  static TimeDuration
  MilliSeconds(double s)
  {
    return (TimeDuration(s));
  }

  /** Return a TimeDuration explicitly defined in sec. */
  static TimeDuration
  Seconds(double s)
  {
    return (TimeDuration(s * 1000.0));
  }

  /** Return a TimeDuration explicitly defined in minutes. */
  static TimeDuration
  Minutes(double m)
  {
    return (Seconds(m * 60.0));
  }

  /** Return a TimeDuration explicitly defined in hours. */
  static TimeDuration
  Hours(double h)
  {
    return (Seconds(h * 60.0 * 60.0));
  }

  /** Return a TimeDuration explicitly defined in days. */
  static TimeDuration
  Days(double d)
  {
    return (Seconds(d * 24.0 * 60.0 * 60.0));
  }

  /** Return a double in milliseconds. */
  double
  milliseconds() const
  {
    return myMSeconds.count();
  }

  /** Return a double in sec. */
  double
  seconds() const
  {
    return myMSeconds.count() / 1000.0;
  }

  /** Return a double in minutes. */
  double
  minutes() const
  {
    return myMSeconds.count() / 1000.0 / 60.0;
  }

  /** Return a double in hours. */
  double
  hours() const
  {
    return myMSeconds.count() / 1000.0 / (60.0 * 60.0);
  }

  /** Return a double in days. */
  double
  days() const
  {
    return myMSeconds.count() / 1000.0 / (60.0 * 60.0 * 24.0);
  }

  /** Multiplication (scaling) */
  friend TimeDuration
  operator * (double,
    const TimeDuration&);

  /** Add TimeDuration to a Time */
  Time
  operator + (const Time&)           const;

  /** Scale TimeDuration */
  TimeDuration
  operator * (double s)              const
  {
    return TimeDuration(myMSeconds.count() * s);
  }

  /** Divide down a TimeDuration */
  TimeDuration
  operator / (double s)              const
  {
    return TimeDuration(myMSeconds.count() / s);
  }

  /** Add two TimeDurations */
  TimeDuration
  operator + (const TimeDuration& t) const
  {
    return TimeDuration(myMSeconds.count() + (t.myMSeconds.count()));
  }

  /** Subtract two TimeDurations */
  TimeDuration
  operator - (const TimeDuration& t) const
  {
    return TimeDuration(myMSeconds.count() - (t.myMSeconds.count()));
  }

  /** Add another TimeDuration to ourselves */
  TimeDuration&
  operator += (const TimeDuration& t)
  {
    myMSeconds += t.myMSeconds;
    return (*this);
  }

  /** Subtract another TimeDuration from ourselves */
  TimeDuration&
  operator -= (const TimeDuration& t)
  {
    myMSeconds -= t.myMSeconds;
    return (*this);
  }

  /** Scale multiple our TimeDuration */
  TimeDuration&
  operator *= (double s)
  {
    myMSeconds *= s;
    return (*this);
  }

  /** Scale divide our TimeDuration */
  TimeDuration&
  operator /= (double s)
  {
    myMSeconds /= s;
    return (*this);
  }

  /** Less than compare two TimeDurations */
  bool
  operator < (const TimeDuration& t) const
  {
    return (myMSeconds < t.myMSeconds);
  }

  /** Less than equal compare two TimeDurations */
  bool
  operator <= (const TimeDuration& t) const
  {
    return (myMSeconds <= t.myMSeconds);
  }

  /** Equality test two TimeDurations */
  bool
  operator == (const TimeDuration& t) const
  {
    return (myMSeconds == t.myMSeconds);
  }

  /** Greater than equal test two TimeDurations */
  bool
  operator >= (const TimeDuration& t) const
  {
    return (myMSeconds >= t.myMSeconds);
  }

  /** Greater than test two TimeDurations */
  bool
  operator > (const TimeDuration& t) const
  {
    return (myMSeconds > t.myMSeconds);
  }

  /** Not equal test two TimeDurations */
  bool
  operator != (const TimeDuration& t) const
  {
    return (myMSeconds != t.myMSeconds);
  }

protected:

  /** Store a chrono with millisecond accuracy */
  std::chrono::milliseconds myMSeconds;
};

/** Output a TimeDuration */
std::ostream      &
operator << (std::ostream&,
  const TimeDuration&);
}
