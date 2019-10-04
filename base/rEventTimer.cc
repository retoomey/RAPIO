#include "rEventTimer.h"

#include "rOS.h"
#include "rError.h"

// System include files
#include <cassert>
#include <cstdlib> // atoi()
#include <ctime>   // time()
#include <iostream>
#include <typeinfo>
#include <chrono>
#include <thread>

using namespace rapio;
using namespace std;
using namespace std::chrono;

EventTimer::EventTimer(size_t milliseconds, const std::string& name) : myDelayMS(milliseconds), myName(name)
{ }

void
EventTimer::setTimerMilliseconds(size_t m)
{
  myDelayMS = m;
}

void
EventTimer::action()
{
  LogSevere("Timer empty action " << (void *) (this) << " " << myName << "\n");
}

void
EventTimer::start(
  std::chrono::time_point<std::chrono::high_resolution_clock> t)
{
  // LogSevere("START CALLED " << (void*)(this) << " " << myDelayMS << "\n");
  myFire = t + std::chrono::milliseconds(myDelayMS);
}

double
EventTimer::readyInMS(
  std::chrono::time_point<std::chrono::high_resolution_clock> at)
{
  // Decreasing until myFire to negative...
  std::chrono::duration<double, std::milli> elapsed = myFire - at;

  // LogSevere("Timer counter " << (void*)(this) << ", "  << elapsed.count() <<
  // "\n");
  return (elapsed.count());
}
