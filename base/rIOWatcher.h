#pragma once

#include <rIO.h>
#include <rEventTimer.h>

#include <string>
#include <memory>

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
class IOListener : public IO {
public:
  /** Handle a new file event for your watch */
  virtual void
  handleNewFile(const std::string& filename){ };

  /** Handle a new directory event for your watch */
  virtual void
  handleNewDirectory(const std::string& dirname){ };

  /** Handle an unmount event for your watch */
  virtual void
  handleUnmount(const std::string& dirname){ };

  /** Handle a poll event for your watch */
  virtual bool
  handlePoll(){ return false; };
};

class WatcherType : public EventTimer {
public:
  WatcherType(size_t milliseconds, const std::string& name) :
    EventTimer(milliseconds, name){ }

  /** Attach this listener to given URL */
  virtual bool
  attach(const std::string& dirname, IOListener * l){ return false; };

  /** Detach this listener from ALL connections */
  virtual void
  detach(IOListener * l){ };
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
