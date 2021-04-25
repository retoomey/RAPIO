#include "rProcessTimer.h"
#include "rOS.h"
#include "rTime.h"
#include "rTimeDuration.h"

#include <fstream>

#include <iostream>
#include <thread>

using namespace rapio;

ProcessTimer::ProcessTimer(const std::string& message) : myMsg(message),
  myStartTime(Time::CurrentTime()),
  myCPUTime(Time::getElapsedCPUTimeMSec())
{ }

TimeDuration
ProcessTimer::getElapsedTime()
{
  // std::this_thread::sleep_for(std::chrono::seconds(1));
  //  std::cout << Time::CurrentTime() << "\n";
  //  std::cout << myStartTime << "\n";
  return (Time::CurrentTime() - myStartTime);
}

int
ProcessTimer::getProgramSize()
{
  int result(0);

  std::ifstream fp("/proc/self/statm");

  if (fp) { // first number in the file
    fp >> result;
  }

  return (result);
}

TimeDuration
ProcessTimer::getCPUTime()
{
  unsigned long cpu_msec = Time::getElapsedCPUTimeMSec() - myCPUTime;

  return (TimeDuration::Seconds(cpu_msec / 1000.0));
}

void
ProcessTimer::print()
{
  if (myMsg != "") {
    LogInfo(myMsg << " : "
                  << getElapsedTime() << " wall-clock time, "
                  << getCPUTime() << " cpu-time, "
                  << getProgramSize() << " pages\n");
  }
}

ProcessTimer::~ProcessTimer()
{
  print();
}
