#pragma once

#include <chrono>
#include <vector>
#include <string>
#include <thread>
#include <atomic>

namespace rapio {
class EventCallback;

/**
 *
 * @ingroup rapio_event
 * @brief Called by EventTimer, handles an event
 */
class EventHandler {
public:

  /** Construct an event handler */
  EventHandler(const std::string& n) : myName(n), myIsReady(false){ }

  /** Called by locked event loop to check if ready */
  bool
  isReady()
  {
    return myIsReady;
  }

  /** Called only by locked event loop to clear ready */
  void
  clearReady()
  {
    myIsReady = false;
  }

  /** Called by thread to request action */
  void
  setReady();

  /** Action callback from the main event loop thread */
  virtual void
  action(){ };

  /** Create a std::thread iff needed/wanted by this handler */
  virtual bool
  createTimerThread(std::vector<std::thread>& threads){ return false; }

  /** New action (taking on thread notify) */
  virtual void
  actionMainThread()
  {
    clearReady();
    action();
  };

  /** Return name of handler */
  std::string
  getName(){ return myName; }

protected:

  /** Name of the timer for debugging, etc. */
  std::string myName;

  /** This timer/thread ready for action */
  std::atomic<bool> myIsReady;
};

/** Event timer class for firing close to given ms.
 *
 * @author Robert Toomey
 * @ingroup rapio_event
 * @brief Timers for synchronous calling in main loop.
 */
class EventTimer : public EventHandler {
public:

  /** Millisecond accuracy timer.  Probably good enough */
  EventTimer(size_t milliseconds, const std::string& name);

  /** Local timer thread function, 'could' use a lambda */
  void
  localMainThread();

  /** Create std::thread if needed */
  virtual bool
  createTimerThread(std::vector<std::thread>& threads) override;

  /** Get the current timer in milliseconds */
  size_t
  getTimerMilliseconds(){ return myDelayMS; }

  /** Change delay time, will effect next run */
  void
  setTimerMilliseconds(size_t m);

  /** Action to take on timer pulse */
  virtual void
  action() override;

public:

  /** Time we want to fire at (calculated from myDelayMS) */
  std::chrono::time_point<std::chrono::high_resolution_clock> myFire;

  /** Heartbeat in milliseconds */
  size_t myDelayMS;
};
}
