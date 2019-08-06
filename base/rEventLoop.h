#pragma once

#include <rEvent.h>

#include <vector>
#include <memory>

namespace rapio {
class EventTimer;
// class EventHandler;

/**
 * Callbacks for main loop to process
 *
 * @see EventLoop
 */

/*
 * class EventCallback : public Event {
 * public:
 *
 ** This is the function that will be invoked when the event happens.  *
 **virtual void actionPerformed() = 0;
 **
 ** Meant to be subclassed. *
 **virtual ~EventCallback() {}
 **};
 */


/** Event loop of the running application */
class EventLoop : public Event {
public:

  /** Add timer to list.  No mechanism for searching/replacing
  * deleting here, but we can add that easy later if wanted */
  static void
  addTimer(std::shared_ptr<EventTimer> t)
  {
    myTimers.push_back(t);
  }

  /** Add event generator to the main loop */
  // static void addEventHandler(std::shared_ptr<EventHandler>c) {
  //   myEventHandlers.push_back(c);
  // }

  /** Start the application main loop */
  static void
  doEventLoop();

  virtual ~EventLoop(){ }

private:

  /** The set of all the event callbacks */
  // static std::vector<EventCallback *>allEventActions;

  /** The amount of time that we sleep when there are no callbacks. */
  static size_t LOOP_IDLE_MS;

  /** Max numbe of call backs we can handle at once */
  static size_t MAX_CALLBACKS;

  /** Things that generate events */
  // static std::vector<std::shared_ptr<EventHandler> >myEventHandlers;

  /** Timer/heartbeats in main loop */
  static std::vector<std::shared_ptr<EventTimer> > myTimers;
};
}
