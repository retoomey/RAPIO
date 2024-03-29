#pragma once

#include <rIOWatcher.h>
#include <rEventLoop.h>
#include <rEventTimer.h>
#include <rURL.h>

#include <sys/stat.h>

namespace rapio {
/** Poller for directories where networked drives
 * or situations where FAM is not available.
 *
 * @author Robert Toomey
 */
class DirWatcher : public WatcherType {
public:
  /** Default constant for a directory watcher */
  static const std::string DIR_WATCH;

  DirWatcher() : WatcherType(5000, 10, "Directory time poll event handler"){ }

  static void
  introduceSelf();

  /** Information for a particular watch */
  class DirInfo : public WatchInfo {
public:
    friend DirWatcher;

    DirInfo(IOListener * l, const std::string& dir)
      : WatchInfo(l), myURL(dir)
    {
      myLastStat.st_ctim.tv_sec  = 0; // good enough
      myLastStat.st_ctim.tv_nsec = 0;
    }

    /** Create the events to be processed later */
    virtual void
    createEvents(WatcherType * w) override;
protected:
    /** The URL of the directory being watched */
    URL myURL;

    /** Stat memory of the latest file checked */
    struct stat myLastStat;
  };

  /** Attach a pulse to web page for a given listener to us */
  virtual bool
  attach(const std::string& dirname, bool realtime, bool archive, IOListener *) override;

  /** Get some events.  Depending on watcher this can be a global process or
   * passed on the individual infos */
  virtual void
  getEvents() override;

  /** Destroy us */
  virtual ~DirWatcher(){ }

private:

  /** Recursive scan for new files during a poll */
  void
  scan(IOListener * l, const std::string& dir, struct stat & lowtime, struct stat & newlowtime);
};
}
