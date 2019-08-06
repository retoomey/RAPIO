#include "rTime.h"

#include <iostream>
#include <sys/timeb.h>
#include <sys/times.h> // struct tms

#include <sys/time.h> // gettimeofday
#include <rError.h>
#include <rSignals.h>

using namespace rapio;

std::string Time::FILE_TIMESTAMP   = "%04d%02d%02d-%02d%02d%02d";
std::string Time::RECORD_TIMESTAMP = "%04d%02d%02d-%02d%02d%02d.%03d";
std::string Time::LOG_TIMESTAMP    = "[%04d %02d/%02d %02d:%02d:%02d UTC]";

std::chrono::system_clock::time_point
Time::toTimepoint(timeval& src)
{
  /*  Clipping to XXX000 micro to milli... */

  /*
   * // trusting the value with only milliseconds accuracy
   * unsigned short t1 = (unsigned short)((src.tv_usec/1000.0)+0.5);
   *
   * using dest_timepoint_type=std::chrono::time_point<
   *  std::chrono::system_clock, std::chrono::milliseconds >;
   * dest_timepoint_type converted{
   *  std::chrono::milliseconds{
   *    //src.tv_sec*1000+src.tv_usec/1000
   *    src.tv_sec*1000+t1
   *  }
   * };
   */

  // Microsecond
  using dest_timepoint_type = std::chrono::time_point<
      std::chrono::system_clock, std::chrono::microseconds>;
  dest_timepoint_type converted {
    std::chrono::microseconds {
      src.tv_sec *1000000 + src.tv_usec
    }
  };

  // this is to make sure the converted timepoint is indistinguishable by one
  // issued by the system clock
  //
  std::chrono::system_clock::time_point recovered =
    std::chrono::time_point_cast<std::chrono::system_clock::duration>(converted)
  ;
  return recovered;
}

timeval
Time::toTimeval(std::chrono::system_clock::time_point now)
{
  struct timeval dest;

  /* MILLISECOND
   * auto millisecs=
   *  std::chrono::duration_cast<std::chrono::milliseconds>(
   *      now.time_since_epoch()
   *  );;
   * dest.tv_sec=millisecs.count()/1000;
   * dest.tv_usec=(millisecs.count()%1000)*1000;
   */

  // MICROSECOND
  auto microsecs =
    std::chrono::duration_cast<std::chrono::microseconds>(
    now.time_since_epoch()
    );

  ;

  dest.tv_sec  = microsecs.count() / 1000000;
  dest.tv_usec = (microsecs.count() % 1000000);

  return dest;
}

Time::Time()
{
  set(0, 0);
}

Time::Time(time_t s, double f)
{
  set(s, f);
}

Time::Time(
  int            year,
  unsigned short month,
  unsigned short day,
  unsigned short hour,
  unsigned short minute,
  unsigned short second
)
{
  tm a;

  a.tm_year  = year - 1900;
  a.tm_mon   = month - 1;
  a.tm_mday  = day;
  a.tm_hour  = hour;
  a.tm_min   = minute;
  a.tm_sec   = second;
  a.tm_isdst = -1;

  // linux I love you, really.  mktime is local only, we need UTC
  // Read the timegm manpage if we need to port to non-linux which
  // I'm not planning on
  time_t retval = timegm(&a);
  myTimepoint = std::chrono::system_clock::from_time_t(retval);
}

Time
Time::SecondsSinceEpoch(time_t t, double f)
{
  return (Time(t, f));
}

Time
Time::LastPossibleTime()
{
  return (Time(std::numeric_limits<time_t>::max(), 0.9999));
}

void
Time::set(time_t epoch_sec, double frac_sec)
{
  timeval holdme;

  holdme.tv_sec  = epoch_sec;
  holdme.tv_usec = frac_sec * 1000000.0;

  myTimepoint = toTimepoint(holdme);
}

// FIXME: We have chrono...
Time
Time::CurrentTime()
{
  // #1 TIMEB    millisecond accuracy (oldest) (MMM)
  // timeb, ftime.  Use .time and .millitm
  // #2 TIMEVAL  microsecond accuracy (newer) (MMMMMM)
  // gettimeofday(&result, 0);
  // #3 TEMPSPEC nanosecond accuracy (newest) -lrt or redhat 7 up I think
  // clock_gettime(CLOCK_REALTIME, &result);
  //

  // #1
  // ftime(&result2);
  // return (SecondsSinceEpoch(result.time, result.millitm / 1000.0));

  // #2
  struct timeval result; // gettimeofday is UTC time

  gettimeofday(&result, 0); // Gives us microseconds (MMMNNN) 0 recommended by docs

  // Round to milli level 521535 microsec --> 522 millisec
  unsigned short milli = (unsigned short) ((result.tv_usec / 1000.0) + 0.5);

  return (SecondsSinceEpoch(result.tv_sec, milli / 1000.0));

  // #3
  // nano-second accuracy
  // This function is available on Linux. Needs librt on the link line.
  // struct timespec result;
  // clock_gettime(CLOCK_REALTIME, &result);
  // static const double NANO = pow(10.0, 9.0);
  // return (SecondsSinceEpoch(result.tv_sec, result.tv_nsec / NANO));
}

std::string
Time::getString(const std::string& pattern, bool milli) const
{
  char buf[80];
  const time_t t = std::chrono::system_clock::to_time_t(myTimepoint);
  tm a = *gmtime(&t);

  if (milli) {
    int millisec = (int) (1000.0 * getFractional() + 0.5);
    if (millisec == 1000) { millisec = 999; } // possible???
    std::snprintf(buf, sizeof(buf), pattern.c_str(),
      a.tm_year + 1900,
      a.tm_mon + 1,
      a.tm_mday,
      a.tm_hour,
      a.tm_min,
      a.tm_sec,
      millisec);
  } else {
    std::snprintf(buf, sizeof(buf), pattern.c_str(),
      a.tm_year + 1900,
      a.tm_mon + 1,
      a.tm_mday,
      a.tm_hour,
      a.tm_min,
      a.tm_sec);
  }
  return (buf);
}

std::ostream&
rapio::operator << (std::ostream& os, const Time& t)
{
  return (os << "(" << t.getSecondsSinceEpoch() << "."
             << t.getFractional()
             << " epoch secs "
             << ")");
}

time_t
Time::getSecondsSinceEpoch() const
{
  auto microsecs =
    std::chrono::duration_cast<std::chrono::microseconds>(
    myTimepoint.time_since_epoch()
    );

  return (microsecs.count() / 1000000);
}

double
Time::getFractional() const
{
  auto microsecs =
    std::chrono::duration_cast<std::chrono::microseconds>(
    myTimepoint.time_since_epoch()
    );

  return (microsecs.count() % 1000000) / 1000000.0;
}

unsigned long
Time::getElapsedCPUTimeMSec()
{
  unsigned long retval = 0ul;
  struct tms t;

  times(&t);
  retval = t.tms_utime + t.tms_stime + t.tms_cutime + t.tms_cstime;
  return (retval);
}

int
Time::getYear() const
{
  const time_t t = std::chrono::system_clock::to_time_t(myTimepoint);
  tm a = *gmtime(&t);

  return a.tm_year + 1900;
}

int
Time::getMonth() const
{
  const time_t t = std::chrono::system_clock::to_time_t(myTimepoint);
  tm a = *gmtime(&t);

  return a.tm_mon + 1;
}

int
Time::getDay() const
{
  const time_t t = std::chrono::system_clock::to_time_t(myTimepoint);
  tm a = *gmtime(&t);

  return a.tm_mday;
}

int
Time::getHour() const
{
  const time_t t = std::chrono::system_clock::to_time_t(myTimepoint);
  tm a = *gmtime(&t);

  return a.tm_hour;
}

int
Time::getMinute() const
{
  const time_t t = std::chrono::system_clock::to_time_t(myTimepoint);
  tm a = *gmtime(&t);

  return a.tm_min;
}

int
Time::getSecond() const
{
  const time_t t = std::chrono::system_clock::to_time_t(myTimepoint);
  tm a = *gmtime(&t);

  return a.tm_sec;
}

Time::Time(const std::string& instr, int offset)
{
  tm myTM;
  static const char * file_fmt = "%04d%02d%02d-%02d%02d%02d";
  const int ret(std::sscanf(instr.c_str(), file_fmt,
    &myTM.tm_year, &myTM.tm_mon, &myTM.tm_mday,
    &myTM.tm_hour, &myTM.tm_min, &myTM.tm_sec));

  // offset is never used.  Let's remove it
  if (ret != 6) {
    // look for two sets: "YYYY MM DD" followed by "HH MM SS"
    // we look for them by walking throug the string looking for digits.
    char ch;
    offset = instr.find_first_of("0123456789", offset != -1 ? offset : 0);
    std::sscanf(instr.c_str() + offset, "%04d%c%02d%c%02d",
      &myTM.tm_year, &ch, &myTM.tm_mon, &ch, &myTM.tm_mday);
    offset += 10;
    offset  = instr.find_first_of("0123456789", offset);
    std::sscanf(instr.c_str() + offset, "%02d%c%02d%c%02d",
      &myTM.tm_hour, &ch, &myTM.tm_min, &ch, &myTM.tm_sec);
  }

  myTM.tm_year = myTM.tm_year - 1900;
  myTM.tm_mon  = myTM.tm_mon - 1;

  if ((myTM.tm_year >= 0) &&
    (myTM.tm_mon >= 0) && (myTM.tm_mon <= 11) &&
    (myTM.tm_mday >= 1) && (myTM.tm_mday <= 31) &&
    (myTM.tm_hour >= 0) && (myTM.tm_hour <= 23) &&
    (myTM.tm_min >= 0) && (myTM.tm_min <= 59) &&
    (myTM.tm_sec >= 0) && (myTM.tm_sec <= 61))
  {
    myTM.tm_isdst = -1;
    time_t retval = timegm(&myTM);
    myTimepoint = std::chrono::system_clock::from_time_t(retval);
  } else {
    // FIXME.  Maybe current time?
  }
}

Time
Time::fromStringForFileName(const std::string& good_name)
{
  return (Time(good_name));
}
