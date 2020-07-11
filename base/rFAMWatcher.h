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

  FAMWatcher() : WatcherType(0, 1, "FAM Watcher"){ }

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

    /** Handle detach of watch */
    virtual bool
    handleDetach(WatcherType * owner) override;
  };

  /** Attach a FAM listener to us */
  virtual bool
  attach(const std::string& dirname, IOListener *) override;

  /** Get some events.  Depending on watcher this can be a global process or
   * passed on the individual infos */
  virtual void
  getEvents() override;

  /** Get the global FAM file descripter */
  static int
  getFAMID(){ return theFAMID; }

private:

  /** Initialize FAM inotify */
  void
  init();

  /** Add FAM Event */
  void
  addFAMEvent(IOListener * l,
    const std::string& d, const std::string& n, const uint32_t amask);

  /** Global FAM file descripter for us */
  static int theFAMID;
};
}
