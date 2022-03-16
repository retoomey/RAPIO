#pragma once

#include <rEvent.h>

#include <vector>
#include <memory>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace rapio {
class EventHandler;

/** Main event loop of the running application,
 * which polls registered synchronous timers to do all our work.
 *
 * @author Robert Toomey
 */
class EventLoop : public Event {
public:

  /** Create an event loop */
  EventLoop(){ }

  /** Add timer to list.  No mechanism for searching/replacing
  * deleting here, but we can add that easy later if wanted */
  static void
  addEventHandler(std::shared_ptr<EventHandler> t)
  {
    myEventHandlers.push_back(t);
  }

  /** Run main event loop */
  static void
  doEventLoop();

  /** Destroy an event loop */
  virtual ~EventLoop(){ }

  // Humm maybe hide these better.  The timers will use these to
  // sync with event loop

  /** Lock for check thread variable and for pulling thread ready states */
  static std::mutex theEventLock;

  /** Conditional variable marking some thread is ready */
  static std::condition_variable theEventCheckVariable;

  /** Ready flag  */
  static bool theReady;

  /** Exit/shutdown with a exit code */
  static void
  exit(int theExitCode);

  /** Thread pool of all running threads */
  static std::vector<std::thread> theThreads;

private:

  /** Exit code to use  */
  static int exitCode;

  /** Boolean are we still running?  */
  static std::atomic<bool> isRunning;

  /** Timer/heartbeats in main loop */
  static std::vector<std::shared_ptr<EventHandler> > myEventHandlers;
};
}
