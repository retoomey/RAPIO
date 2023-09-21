#include "rProcessTimer.h"
#include "rTimeDuration.h"
#include "rOS.h"
#include "rStrings.h"

using namespace rapio;

ProcessTimer::ProcessTimer(const std::string& message) : myMsg(message),
  myTimer()
{
  OS::getProcessSizeKB(myVM_KB, myRSS_KB);

  // Remove newline characters from the end of the string
  myHaveNewline = false;
  while (!myMsg.empty() && myMsg.back() == '\n') {
    myMsg.pop_back();
    myHaveNewline = true;
  }
}

void
ProcessTimer::reset()
{
  myTimer = boost::timer::cpu_timer();
  OS::getProcessSizeKB(myVM_KB, myRSS_KB);
}

double
ProcessTimer::getVirtualKB()
{
  double vmkb, rsskb;

  OS::getProcessSizeKB(vmkb, rsskb);
  return (vmkb - myVM_KB);
}

double
ProcessTimer::getResidentKB()
{
  double vmkb, rsskb;

  OS::getProcessSizeKB(vmkb, rsskb);
  return (rsskb - myRSS_KB);
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

ProcessTimerSum::ProcessTimerSum(const std::string& message) : myMsg(message),
  myCounter(0)
{
  OS::getProcessSizeKB(myVM_KB, myRSS_KB);

  // Remove newline characters from the end of the string
  myHaveNewline = false;
  while (!myMsg.empty() && myMsg.back() == '\n') {
    myMsg.pop_back();
    myHaveNewline = true;
  }
}

void
ProcessTimerSum::
add(const ProcessTimer& timer)
{
  // Get all the info at once
  boost::timer::cpu_times elapsed = timer.myTimer.elapsed();

  // Accumulate passed times
  mySystem += TimeDuration::MilliSeconds((elapsed.system) / 1e6);
  myUser   += TimeDuration::MilliSeconds((elapsed.user) / 1e6);
  myWall   += TimeDuration::MilliSeconds((elapsed.wall) / 1e6);

  // Accumulate CPU percentage (to average)
  std::string cpuStr = timer.myTimer.format(6, "%p");

  try{
    float cpu = std::stof(cpuStr);
    myCPU += cpu;
  }catch (...) {
    // don't care keep it 0 then
  }

  myCounter++;
}

namespace rapio {
// Common pretty printing for timers
void
printTimerLine(std::ostream& os,
  const TimeDuration& wall,
  const TimeDuration& user,
  const TimeDuration& system,
  const float cpuPercent,
  const std::string& msg, double myVM_KB, double myRSS_KB,
  bool newline)
{
  // Get the current memory and color format the change in memory
  double vmkb, rsskb;

  OS::getProcessSizeKB(vmkb, rsskb);
  const double currentVM  = vmkb * 1024;
  const double currentRSS = rsskb * 1024;
  const double changeVM   = (vmkb - myVM_KB) * 1024;
  const double changeRSS  = (rsskb - myRSS_KB) * 1024;
  const std::string c1    = changeVM > 0 ? ColorTerm::fRed : ColorTerm::fGreen;
  const std::string c2    = changeRSS > 0 ? ColorTerm::fRed : ColorTerm::fGreen;
  const std::string e     = ColorTerm::fNormal;

  // A standard output message
  os << msg << ": " <<
    "CPU: wall" << wall << " user" << user << " sys" << system <<
    " (" << cpuPercent << "%)" <<
    " RAM:" <<
    " v[" << Strings::formatBytes(currentVM) <<
    " " << c1 << Strings::formatBytes(changeVM, true) << e << "]" <<
    " rss[" << Strings::formatBytes(currentRSS) <<
    " " << c2 << Strings::formatBytes(changeRSS, true) << e << "]"
     << (newline ? "\n" : "");
}

std::ostream&
operator << (std::ostream& os, const rapio::ProcessTimer& pt)
{
  // Convert to our time class
  std::stringstream temp;
  boost::timer::cpu_times elapsed = pt.myTimer.elapsed();
  auto wall   = TimeDuration::MilliSeconds((elapsed.wall) / 1e6);
  auto user   = TimeDuration::MilliSeconds((elapsed.user) / 1e6);
  auto system = TimeDuration::MilliSeconds((elapsed.system) / 1e6);

  // CPU usage
  std::string cpuStr = pt.myTimer.format(6, "%p");
  float cpu = 0.0;

  try{
    cpu = std::stof(cpuStr);
  }catch (...) {
    // don't care keep it 0 then
  }

  // And print
  printTimerLine(os,
    wall, user, system, cpu,
    pt.myMsg, pt.myVM_KB, pt.myRSS_KB,
    pt.myHaveNewline);

  return os;
} // <<

std::ostream&
operator << (std::ostream& os, const rapio::ProcessTimerSum& t)
{
  if (t.myCounter > 0) {
    // Average CPU
    float cpuPercent = (t.myCPU / t.myCounter);

    printTimerLine(os,
      t.myWall, t.myUser, t.mySystem, cpuPercent,
      t.myMsg, t.myVM_KB, t.myRSS_KB,
      t.myHaveNewline);
  } else {
    LogInfo(t.myMsg << " (no processes added) " << (t.myHaveNewline ? "\n" : ""));
  }
  return os;
}
}
