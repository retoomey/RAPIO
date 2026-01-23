#include "rEventTimer.h"

#include "rOS.h"
#include "rError.h"
#include "rEventLoop.h"

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

EventTimer::EventTimer(size_t milliseconds, const std::string& name) : EventHandler(name), myDelayMS(milliseconds)
{ }

void
EventTimer::localMainThread()
{
  // Don't Log here, only the main loop is synced
  // To log here log would have to be mutex locked to print
  // which I'm avoiding for speed reasons (only print in the action)
  // You can still use std::cout if debugging but it will probably intermix with other
  // output

  // hack for moment
  if (myDelayMS == 0) {
    std::cout << "Not hammering..these need to change... " << myName << "\n";
    myDelayMS = 1000;
  }

  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(myDelayMS));

    // Fire ready so action is called
    setReady();
  }
}

bool
EventTimer::createTimerThread(std::vector<std::thread>& threads)
{
  threads.push_back(std::thread(&EventTimer::localMainThread, this));
  return true;
}

void
EventHandler::setReady()
{
  // Only update ready and notify on event lock
  // We could probably pass our ready flag let event loop handle locking?
  std::lock_guard<std::mutex> lck(EventLoop::theEventLock);

  // We're already ready, event will turn us off when it has the lock
  // So we don't need to do anything...
  if (myIsReady == true) {
    // Do nada
  } else {
    myIsReady = true; // Do during lock or we might get 'double' called for work
    EventLoop::theReady = true;
    EventLoop::theEventCheckVariable.notify_one();
  }
}

void
EventTimer::setTimerMilliseconds(size_t m)
{
  myDelayMS = m;
}

void
EventTimer::action()
{
  fLogSevere("Timer empty action {} {}", (void *) (this), myName);
}
