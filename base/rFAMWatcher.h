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
 * @author Robert Toomey
 */
class FAMWatcher : public WatcherType {
public:

  FAMWatcher() : WatcherType(0, "FAM Watcher"){ }

  /** Introduce this to the global factory */
  static void
  introduceSelf();

  /** Store the watch for a particular directory */
  class FAMInfo : public WatchInfo {
public:
    std::string myDirectory;
    int myWatchID;

    FAMInfo(IOListener * l, const std::string& dir, int wd)
      : myDirectory(dir), myWatchID(wd)
    {
      myListener = l;
    }
  };

  /** We store a vector of events */
  class FAMEvent : public WatchEvent {
public:

    /** Create a FAM Event for notifying listeners */
    FAMEvent(IOListener * l,
      const std::string& d, const std::string& n, const uint32_t amask)
      : myListener(l), myDirectory(d), myShortFile(n), myMask(amask){ }

    /** Handle the event action */
    void
    handleEvent();

private:

    /** Listener to handle the event */
    IOListener * myListener;

    /** Directory of file */
    std::string myDirectory;

    /** Short file name */
    std::string myShortFile;

    /** FAM mask of event */
    uint32_t myMask;
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
  std::vector<std::shared_ptr<FAMInfo> > myFAMWatches;

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
