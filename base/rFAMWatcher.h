#pragma once

#include <rIOWatcher.h>

#include <rURL.h>

#include <sys/inotify.h>

#include <memory>
#include <queue>

namespace rapio {
/**
 * Event handler that monitors file creation using the inotify device.
 *
 */
class FAMWatcher : public WatcherType {
public:

  FAMWatcher() : WatcherType(0, "FAM Watcher"){ }

  static void
  introduceSelf();

  /** Store the watch */
  class FAMInfo : public IO {
public:
    IOListener * myListener;
    std::string myDirectory;
    int myWatchID;

    FAMInfo(IOListener * l, const std::string& dir, int wd)
      : myListener(l), myDirectory(dir), myWatchID(wd){ }
  };

  /** We store a vector of events */
  class FAMEvent : public IO {
public:

    FAMEvent(IOListener * l,
      const std::string& d, const std::string& n, const uint32_t amask)
      : myListener(l), myDirectory(d), name(n), mask(amask){ }

    std::string
    getFullName() const
    {
      return (myDirectory + '/' + name);
    }

    std::string
    getDirectory() const
    {
      return myDirectory;
    }

    IOListener * myListener;
    std::string myDirectory;
    std::string name;
    uint32_t mask;
  };

  /** Attach a FAM listener to us */
  virtual bool
  attach(const std::string& dirname, IOListener *) override;

  /** Detach all references for a given FAM listener from us */
  virtual void
  detach(IOListener *) override;

  /** Action to take on timer pulse */
  virtual void
  action() override;

protected:

  /** The list of watches we currently have */
  std::vector<FAMInfo> myWatches;

private:

  /** Get new FAM events into the queue */
  void
  getEvents();

  /** Initialize FAM inotify */
  void
  init();

  /** Global FAM file descripter for us */
  static int theFAMID;
};
}
