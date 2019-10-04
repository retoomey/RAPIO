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

public:

  /// The default interval has zero length.
  // not sure s*1000.0 is rounding correctly
  TimeDuration(double s = 0)
    : myMSeconds(std::chrono::milliseconds((long)(s * 1000.0)))
  { }

  /// Return a TimeDuration explicitly defined in sec.
  static TimeDuration
  Seconds(double s)
  {
    return (TimeDuration(s));
  }

  /// Return a TimeDuration explicitly defined in minutes.
  static TimeDuration
  Minutes(double m)
  {
    return (Seconds(m * 60));
  }

  /// Return a TimeDuration explicitly defined in hours.
  static TimeDuration
  Hours(double h)
  {
    return (Seconds(h * 60 * 60));
  }

  /// Return a TimeDuration explicitly defined in days.
  static TimeDuration
  Days(double d)
  {
    return (Seconds(d * 24 * 60 * 60));
  }

  /// Return a double in milliseconds.
  double
  milliseconds() const
  {
    return myMSeconds.count();
  }

  /// Return a double in sec.
  double
  seconds() const
  {
    return myMSeconds.count() / 1000.0;
  }

  /// Return a double in minutes.
  double
  minutes() const
  {
    return myMSeconds.count() / 1000.0 / 60.0;
  }

  /// Return a double in hours.
  double
  hours() const
  {
    return myMSeconds.count() / 1000.0 / (60 * 60);
  }

  /// Return a double in days.
  double
  days() const
  {
    return myMSeconds.count() / 1000.0 / (60 * 60 * 24);
  }

  /// multiplication (scaling) operator for LHS double
  friend TimeDuration
  operator * (double,
    const TimeDuration&);

  Time
  operator + (const Time&)           const;
  TimeDuration
  operator * (double s)              const
  {
    return (myMSeconds.count() / 1000.0) * s;
  }

  TimeDuration
  operator / (double s)              const
  {
    return (myMSeconds.count() / 1000.0) / s;
  }

  double
  operator / (const TimeDuration& t) const
  {
    return (myMSeconds.count() / 1000.0) / (t.myMSeconds.count() / 1000.0);
  }

  TimeDuration
  operator + (const TimeDuration& t) const
  {
    return (myMSeconds.count() / 1000.0) + (t.myMSeconds.count() / 1000.0);
  }

  TimeDuration
  operator - (const TimeDuration& t) const
  {
    return (myMSeconds.count() / 1000.0) - (t.myMSeconds.count() / 1000.0);
  }

  TimeDuration&
  operator += (const TimeDuration& t)
  {
    myMSeconds += t.myMSeconds;
    return (*this);
  }

  TimeDuration&
  operator -= (const TimeDuration& t)
  {
    myMSeconds -= t.myMSeconds;
    return (*this);
  }

  TimeDuration&
  operator *= (double s)
  {
    myMSeconds *= s; // do we need to scale?
    return (*this);
  }

  TimeDuration&
  operator /= (double s)
  {
    myMSeconds /= s; // do we need to scale?
    return (*this);
  }

  bool
  operator < (const TimeDuration& t) const
  {
    return (myMSeconds < t.myMSeconds);
  }

  bool
  operator <= (const TimeDuration& t) const
  {
    return (myMSeconds <= t.myMSeconds);
  }

  bool
  operator == (const TimeDuration& t) const
  {
    return (myMSeconds == t.myMSeconds);
  }

  bool
  operator >= (const TimeDuration& t) const
  {
    return (myMSeconds >= t.myMSeconds);
  }

  bool
  operator > (const TimeDuration& t) const
  {
    return (myMSeconds > t.myMSeconds);
  }

  bool
  operator != (const TimeDuration& t) const
  {
    return (myMSeconds != t.myMSeconds);
  }

protected:

  /** Store a chrono */
  std::chrono::milliseconds myMSeconds;
};

const TimeDuration&
operator + (const TimeDuration& ti);

TimeDuration
operator - (const TimeDuration& ti);

std::ostream      &
operator << (std::ostream&,
  const TimeDuration&);
}
