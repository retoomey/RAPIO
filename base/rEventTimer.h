#pragma once

#include <rEvent.h>

#include <chrono>
#include <vector>
#include <string>

namespace rapio {
class EventCallback;


/** Event timer class for firing close to given ms.
 * These are added to the main event loop to do/check some action
 * on a timer action.  Note the timer is fuzzy here.
 * @author Robert Toomey */
class EventTimer : public Event {
public:

  /** Millisecond accuracy timer.  Probably good enough */
  EventTimer(size_t milliseconds, const std::string& name);

  /** Get the current timer in milliseconds */
  size_t
  getTimerMilliseconds(){ return myDelayMS; }

  /** Change delay time, will effect next run */
  void
  setTimerMilliseconds(size_t m);

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

  /** Time we want to fire at (calculated from myDelayMS) */
  std::chrono::time_point<std::chrono::high_resolution_clock> myFire;

  /** Heartbeat in milliseconds */
  size_t myDelayMS;

  /** Name of the timer for debugging, etc. */
  std::string myName;
};
}
