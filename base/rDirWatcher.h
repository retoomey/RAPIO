#pragma once

#include <rIOWatcher.h>
#include <rEventLoop.h>
#include <rEventTimer.h>
#include <rURL.h>

#include <sys/stat.h>

namespace rapio {
/** Poller for directories where networkd drives
 * or situations where FAM is not available.
 *
 * @author Robert Toomey
 */
class DirWatcher : public WatcherType {
public:
  DirWatcher() : WatcherType(5000, "Directory time poll event handler"){ }

  static void
  introduceSelf();

  /** Information for a particular watch */
  class DirInfo : public WatchInfo {
public:
    URL myURL;
    struct stat myLastStat;

    DirInfo(IOListener * l, const std::string& dir)
    // : myListener(l), myURL(dir)
      : myURL(dir)
    {
      myListener = l;
      myLastStat.st_ctim.tv_sec  = 0; // good enough
      myLastStat.st_ctim.tv_nsec = 0;
    }

    /** Create the events to be processed later */
    virtual void
    createEvents(WatcherType * w) override;
  };

  /** We store a vector of events */
  class DirWatchEvent : public IO {
public:

    /** Create a Dir Event for notifying listeners */
    DirWatchEvent(IOListener * l,
      const std::string        & file)
      : myListener(l), myFile(file){ }

    /** Handle the event action */
    void
    handleEvent();

private:

    /** Listener to handle the event */
    IOListener * myListener;

    /** New file to process */
    std::string myFile;
  };

  /** Attach a pulse to web page for a given listener to us */
  virtual bool
  attach(const std::string& dirname, IOListener *) override;

  /** Detach all references for a given listener from us */
  virtual void
  detach(IOListener *) override;

  /** Action to take on timer pulse */
  virtual void
  action() override;

  /** Destroy us */
  virtual ~DirWatcher(){ }

private:

  /** Recursive scan for new files during a poll */
  void
  scan(IOListener * l, const std::string& dir, struct stat& lowtime, struct stat& newlowtime);

  /** Get new WEB events into the queue */
  void
  getEvents();
};
}
