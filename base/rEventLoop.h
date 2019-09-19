#pragma once

#include <rEvent.h>

#include <vector>
#include <memory>

namespace rapio {
class EventTimer;

/** Main event loop of the running application,
 * which polls registered synchronous timers to do all our work.
 *
 * @author Robert Toomey
 */
class EventLoop : public Event {
public:

  /** Add timer to list.  No mechanism for searching/replacing
  * deleting here, but we can add that easy later if wanted */
  static void
  addTimer(std::shared_ptr<EventTimer> t)
  {
    myTimers.push_back(t);
  }

  /** Start the application main loop */
  static void
  doEventLoop();

  /** Create an event loop */
  EventLoop(){ }

  /** Destroy an event loop */
  virtual ~EventLoop(){ }

private:

  /** Timer/heartbeats in main loop */
  static std::vector<std::shared_ptr<EventTimer> > myTimers;
};
}
