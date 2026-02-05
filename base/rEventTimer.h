#pragma once
#include <rEventLoop.h>
#include <rBOOST.h>

BOOST_WRAP_PUSH
#include <boost/asio.hpp>
BOOST_WRAP_POP

#include <atomic>
#include <string>
#include <memory>

namespace rapio {
/* Base class handles the "Manual Trigger" logic (setReady)
 * This supports RecordQueue, WebMessageQueue, etc.
 *
 * @author Robert Toomey
 * @ingroup rapio_event
 * @brief Handles a callable action
 * @see EventTimer
 */
class EventHandler : public std::enable_shared_from_this<EventHandler> {
public:

  /** Create an EventHandler class with a given name */
  EventHandler(const std::string& name)
    : myName(name), isScheduled(false){ }

  /** Default destructor */
  virtual
  ~EventHandler() = default;

  /* Called by EventLoop::doEventLoop() before running.
   * Default is to do nothing (passive handlers).
   */
  virtual void start(){ }

  /** Action to take on timer pulse */
  virtual void
  action() = 0;

  /** Return name of handler */
  std::string getName(){ return myName; }

  /** Called by EventHandler to request immediate action */
  void
  setReady()
  {
    bool expected = false;

    // Prevent flooding: only post if not already scheduled
    if (isScheduled.compare_exchange_strong(expected, true)) {
      boost::asio::post(EventLoop::io_context(), [self = shared_from_this()]() {
          self->executeAction();
        });
    }
  }

protected:

  /** Name of the timer for debugging, etc. */
  std::string myName;

private:

  /** Calls the action of the EventHandler */
  void
  executeAction()
  {
    isScheduled.store(false); // Reset flag before running so it can be re-triggered
    action();
  }

  /** Are we scheduled to run? */
  std::atomic<bool> isScheduled;
};


/** Periodic Timer class
 * This adds the "Looping" logic on top of the base EventHandler
 *
 * @author Robert Toomey
 * @ingroup rapio_event
 * @brief An EventHandler that handles a timed event
 */
class EventTimer : public EventHandler {
public:

  /** Create an EventTimer */
  EventTimer(size_t milliseconds, const std::string& name)
    : EventHandler(name),
    myDelay(milliseconds),
    myTimer(EventLoop::io_context())
  { }

  /** Override start to kick off the periodic timer */
  virtual void
  start() override
  {
    if (myDelay > 0) {
      scheduleTick();
    }
  }

  /** Default action logs a warning if not overridden (matches your old code) */
  virtual void
  action() override
  {
    std::cerr << "Timer empty action " << myName << std::endl;
  }

private:

  /** Schedule the timer and refire */
  void
  scheduleTick()
  {
    myTimer.expires_after(std::chrono::milliseconds(myDelay));

    myTimer.async_wait([self =
      std::static_pointer_cast<EventTimer>(shared_from_this())](const boost::system::error_code& ec) {
        if (!ec) {
          // We use setReady() to actually run the action.
          // This consolidates execution logic.
          self->setReady();

          // Re-arm the timer
          self->scheduleTick();
        }
      });
  }

  /** Heartbeat in milliseconds */
  size_t myDelay;

  /** Underlaying boost timer used */
  boost::asio::steady_timer myTimer;
};
}
