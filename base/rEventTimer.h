#pragma once

#include <rEvent.h>

#include <chrono>
#include <vector>
#include <string>

namespace rapio {
class EventCallback;


/** Event timer class for firing close to given ms */
class EventTimer : public Event {
public:

  /** Millisecond accuracy timer.  Probably good enough */
  EventTimer(size_t milliseconds, const std::string& name);

  /** Action to take on timer pulse */
  virtual void
  action();

  /** Reset timer to given now */
  void
  start(std::chrono::time_point<std::chrono::high_resolution_clock> t);

  /** Time until we're ready in milliseconds */
  virtual double
  readyInMS(
    std::chrono::time_point<std::chrono::high_resolution_clock> at);

public:
  // private:

  /** Time we want to fire at (calculated from myDelayMS) */
  std::chrono::time_point<std::chrono::high_resolution_clock> myFire;

  /** Heartbeat in milliseconds */
  size_t myDelayMS;

  /** Name of the timer for debugging, etc. */
  std::string myName;
};

/** Thing that gives us events to process @see EventLoop */

/*
 * class EventHandler : public EventTimer {
 * public:
 *
 * EventHandler():EventTimer(0) { }
 * virtual void addCallbacks(std::vector<EventCallback *>&) = 0;
 * };
 */
}
