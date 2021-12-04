#include "rProcessTimer.h"
#include "rTimeDuration.h"

using namespace rapio;

ProcessTimer::ProcessTimer(const std::string& message) : myMsg(message),
  myTimer()
{ }

void
ProcessTimer::reset()
{
  myTimer = boost::timer::cpu_timer();
}

TimeDuration
ProcessTimer::getWallTime()
{
  boost::timer::cpu_times elapsed = myTimer.elapsed();

  return (TimeDuration::MilliSeconds((elapsed.wall) / 1e6));
}

TimeDuration
ProcessTimer::getUserTime()
{
  boost::timer::cpu_times elapsed = myTimer.elapsed();

  return (TimeDuration::MilliSeconds(elapsed.user / 1e6));
}

TimeDuration
ProcessTimer::getSystemTime()
{
  boost::timer::cpu_times elapsed = myTimer.elapsed();

  return (TimeDuration::MilliSeconds((elapsed.system) / 1e6));
}

TimeDuration
ProcessTimer::getCPUTime()
{
  boost::timer::cpu_times elapsed = myTimer.elapsed();

  return (TimeDuration::MilliSeconds((elapsed.user + elapsed.system) / 1e6));
}

ProcessTimer::~ProcessTimer()
{
  if (myMsg != "") {
    LogInfo(*this << "\n");
  }
}

void
ProcessTimerSum::
add(const ProcessTimer& timer)
{
  // Get all the info at once
  boost::timer::cpu_times elapsed = timer.myTimer.elapsed();

  myWall   += TimeDuration::MilliSeconds((elapsed.wall) / 1e6);
  myUser   += TimeDuration::MilliSeconds((elapsed.user) / 1e6);
  mySystem += TimeDuration::MilliSeconds((elapsed.system) / 1e6);
  myCounter++;
}

namespace rapio {
std::ostream&
operator << (std::ostream& os, const rapio::ProcessTimer& t)
{
  return (os << t.myMsg << " : " << t.myTimer.format(6, "[%w s] wall [%u s] user + [%s s] system = [%t s] CPU (%p%)"));
}

std::ostream&
operator << (std::ostream& os, const rapio::ProcessTimerSum& t)
{
  return (os << t.myWall << " Wall " << t.myUser << " User + " << t.mySystem << " System = " << t.myUser + t.mySystem
             << " CPU");
}
}
