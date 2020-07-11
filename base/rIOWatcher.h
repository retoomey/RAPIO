#pragma once

#include <rIO.h>
#include <rEventTimer.h>

#include <string>
#include <memory>
#include <queue>

namespace rapio {
/** Class which allows watching of IO events
 *
 * Three subclasses at least probably....
 *
 * 1. FAM implementation for getting events from system fam
 * 2. A polling implementation for places FAM won't work (haven't done yet)
 * 3. Web implementation for remote 'directory' maybe...
 *
 * @author Robert Toomey
 */
class WatchInfo;

/** Watch event.  Pretty sure 99% of situations can
 * be handled with a message string and/or data buffer.
 * This keeps us from having to generate a bunch of
 * subclasses.
 *
 * @author Robert Toomey
 */
class IOListener;
class WatchEvent : public virtual IO {
public:
  /** Create empty */
  WatchEvent(){ }

  /** Create a string message for listener */
  WatchEvent(IOListener * l, const std::string& m, const std::string& d) :
    myListener(l), myMessage(m), myData(d){ }

  /** Send event to our listener to actually process */
  void
  handleEvent();

  /** Listener to handle the event */
  IOListener * myListener;

  /** A message from a IOWatcher */
  std::string myMessage;

  /** Possible data as a string, depending on message */
  std::string myData;

  /** Possible data as a char buffer, depending on message */
  std::vector<char> myBuffer;
};

class IOListener : public IO {
public:

  /** Handle a poll event for your watch, basically some listeners aren't
   * using events, such as the web index.  Maybe it should. */
  virtual bool
  handlePoll(){ return false; };

  /** Handle a general watch event */
  virtual void
  handleNewEvent(WatchEvent * event){ };
};

class WatcherType : public EventTimer {
public:
  WatcherType(size_t milliseconds, size_t process, const std::string& name) :
    EventTimer(milliseconds, name), myMaxQueueSize(1000), myWaitWhenQueueFull(true),
    myProcessCount(process){ }

  /** Attach this listener to given URL */
  virtual bool
  attach(const std::string& dirname, IOListener * l){ return false; };

  /** Default action to take on timer pulse, filling and processing our queue */
  virtual void
  action() override;

  /** Detach this listener from ALL connections */
  virtual void
  detach(IOListener * l);

  /** Add new event to queue for processing later */
  virtual void
  addEvent(WatchEvent& e){ myEvents.push(e); };

  /** Create all new events, this can be per info or globally controlled */
  virtual void
  createNewEvents();

  /** Throttle queue if too full too fast to allow other watchers their turns. */
  virtual bool
  queueIsThrottled();

  /** Get some events.  Depending on watcher this can be a global process or
   * passed on the individual infos */
  virtual void
  getEvents(){ };

  /** Process some events from queue */
  virtual void
  processEvents();

protected:

  /** The list of watches we currently have */
  std::vector<std::shared_ptr<WatchInfo> > myWatches;

  /** The queue of events to process we currently have. */
  std::queue<WatchEvent> myEvents;

  /** Max queue size before throttle or warning.  This means
   * the system isn't keeping up */
  size_t myMaxQueueSize;

  /** Wait when queue is full before generating new events? */
  bool myWaitWhenQueueFull;

  /** Process count.  Max events to process at once.  We hog the system
   * here.  Tweak based on what we're doing */
  bool myProcessCount;
};

/** Root class for watcher information */
class WatchInfo : public IO {
public:
  IOListener * myListener;

  /** Handle detach of this WatchInfo, return true to remove */
  virtual bool
  handleDetach(WatcherType * owner){ return true; }

  /** Create the events to be processed later */
  virtual void
  createEvents(WatcherType * w){ };
};

class IOWatcher : public IO {
public:
  IOWatcher(){ }

  // Registering of classes ---------------------------------------------

  /** Introduce self into factories */
  static void
  introduceSelf();

  /** Use this to introduce new subclasses. */
  static void
  introduce(const std::string    & protocol,
    std::shared_ptr<WatcherType> factory);

  /** Returns the watcher for this type */
  static std::shared_ptr<WatcherType>
  getIOWatcher(const std::string& type);
};
}
