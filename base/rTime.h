#pragma once

#include <rData.h>
#include <rTimeDuration.h>

// #include <ctime>
#include <string>
#include <chrono>

namespace rapio {
/**
 * Simple representation of a point on the universal time-line.
 *
 * Time contains seconds since 1970-Jan-01-00:00:00, a
 * special date otherwise known as "epoch". It wraps around
 * std::chrono which can be difficult for students, etc. to use
 * easily. Also the standard keeps evolving so we can hide
 * details with this wrapper class and use newer c+20 stuff
 * later on.
 *
 * Currently keeping millisecond accuracy.
 *
 * @see TimeDuration
 */
class Time : public Data {
public:

  /** The standard file format string date with a - we use for writing files such as  20010228-160316 */
  static std::string FILE_TIMESTAMP;

  /** The standard format string date of form [date UTC] we use for logging or xml, etc. */
  static std::string LOG_TIMESTAMP;

  /** The standard format string used by Record for time stamps. */
  static std::string RECORD_TIMESTAMP;

private:
  /** Store a timepoint, size of 8 */
  std::chrono::system_clock::time_point myTimepoint;
public:

  /** The default time is epoch; that is, 1970 Jan 01 00:00:00. */
  Time();

  /** Construct from epoch and fractional time */
  Time(time_t s, double f = 0);

  /**  Make a Time from a list of integers (wrt UTC).
   *
   *   @param int             year of "common era"
   *
   *   @param unsigned_short  month  (1--12) of the year
   *   @param unsigned_short  day    (1--31) of the month
   *   @param unsigned_short  hour   (0--23) of the day
   *   @param unsigned_short  minute (0--59) of the hour
   *
   *   @param unsigned_short  second (0--61) of the minute.  Values
   *        beyond 59 allow for leap seconds.
   *
   *   @param optional milliseconds, defaults to 0.0
   */
  Time(
    int,
    unsigned short,
    unsigned short,
    unsigned short,
    unsigned short,
    unsigned short,
    double = 0.0
  );

  /**  Convert from a string.
   *
   *   @param string  22 characters formatted as
   *                  [2001-02/28-16:00:00-UTC] or
   *                  [2001 02/28 16:00:00 UTC]
   */
  Time(const std::string&, int offset = -1);

  /** Understands the filename format used by the getStringForFileName
   *  function and converts back to a Date.
   * The input should look like: 20010228-160316
   * FIXME: Just make constructor?
   */
  static Time
  fromStringForFileName(const std::string&);

  /** Destruction doesn't need anything extra */
  ~Time(){ }

  /** Return a Time explicitly defined in UTC epoch seconds. */
  static Time
  SecondsSinceEpoch(time_t t, double f = 0);

  /** Return Time corresponding to current state of system clock. */
  static Time
  CurrentTime();

  /**
   * Return the last possible time representable by this Time object.
   * On 32-bit machines, for example, this would correspond to
   * [2038-01/19-03:14:07-UTC] plus a fraction of 0.9999 seconds.
   */
  static Time
  LastPossibleTime();

  /** Utility function used by process timer */
  static unsigned long
  getElapsedCPUTimeMSec();

  /** Less-than comparison operator */
  bool
  operator < (const Time& t) const { return (myTimepoint < t.myTimepoint); }

  /** Greater-than comparison operator */
  bool
  operator > (const Time& t) const { return (myTimepoint > t.myTimepoint); }

  /** Less-than-or-equal-to comparison operator */
  bool
  operator <= (const Time& t) const { return (myTimepoint <= t.myTimepoint); }

  /** Greater-than-or-equal-to comparison operator */
  bool
  operator >= (const Time& t) const { return (myTimepoint >= t.myTimepoint); }

  /** Equality comparison operator */
  bool
  operator == (const Time& t) const { return (myTimepoint == t.myTimepoint); }

  /** Inequality comparison operator */
  bool
  operator != (const Time& t) const { return (myTimepoint != t.myTimepoint); }

  /** Subtract two times create a duration */
  TimeDuration
  operator - (const Time& b) const
  {
    auto delta = std::chrono::duration_cast<std::chrono::seconds>(myTimepoint - b.myTimepoint);

    return TimeDuration::Seconds(delta.count());
  }

  /** Additive assignment */
  Time&
  operator += (const TimeDuration& ti)
  {
    myTimepoint += ti.myMSeconds;
    return (*this);
  }

  /** Addition operator */
  Time
  operator + (const TimeDuration& ti) const
  {
    Time tmp(*this);

    return (tmp += ti);
  }

  /** Subtractive assignment */
  Time&
  operator -= (const TimeDuration& ti)
  {
    myTimepoint -= ti.myMSeconds;
    return (*this);
  }

  /// subtraction operator
  Time
  operator - (const TimeDuration& ti) const
  {
    Time tmp(*this);

    return (tmp -= ti);
  }

  /** Set a timepoint given epoch seconds and a fractional of a full second */
  void
  set(time_t t, double f);

  // Utilities to get time back as a string

  /**  Gets string output from time passing in a pattern string */
  std::string
  getString(const std::string&, bool wantMilli = false) const;

  /** Convenience function */
  std::string
  getLogString() const
  {
    return getString(LOG_TIMESTAMP);
  };

  /** Convenience function */
  std::string
  getFileNameString(bool milli = true) const
  {
    if (milli) {
      return getString(RECORD_TIMESTAMP, true);
    }
    return getString(FILE_TIMESTAMP);
  };

  /** Convenience function */
  std::string
  getRecordTimeString() const
  {
    return getString(RECORD_TIMESTAMP, true);
  };

  // Get part of a time object routines -----

  /** Return the seconds since epoch 0 */
  time_t
  getSecondsSinceEpoch() const;

  /** @return the fraction of a second part of this Time */
  double
  getFractional() const;

  /** @return the (UTC) year. */
  int
  getYear() const;

  /** Return the (UTC) month (1--12) of the year. */
  int
  getMonth() const;

  /**  Return the (UTC) day (1--31) of the month. */
  int
  getDay() const;

  /** Return the (UTC) hour (0--23) of the day. */
  int
  getHour() const;

  /** Return the minute (0--59) of the hour. */
  int
  getMinute() const;

  /** Return the second (0--61) of the minute. */
  int
  getSecond() const;

  // Conversion routines...

  /** Static from timeval to chrono time point conversion */
  static std::chrono::system_clock::time_point
  toTimepoint(timeval& src);

  /** Static from chrono time point to timeval conversion */
  static timeval
  toTimeval(std::chrono::system_clock::time_point now);
};

std::ostream&
operator << (std::ostream&,
  const rapio::Time&);
}
