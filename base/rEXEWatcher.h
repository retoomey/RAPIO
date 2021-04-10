#pragma once

#include <rIOWatcher.h>
#include <rEventLoop.h>
#include <rEventTimer.h>
#include <rURL.h>

#include <vector>
#include <string>

#include <spawn.h> // posix_spawn
#include <sys/stat.h>

namespace rapio {
/** Poller for external executables, piping cout and cerr
 * into processable events.
 * This allows you to stream data from external processes
 * into your class.
 *
 * @author Robert Toomey
 */
class EXEWatcher : public WatcherType {
public:
  /** Default constant for a exe watcher */
  static const std::string EXE_WATCH;

  EXEWatcher() : WatcherType(100, 2, "EXE Watcher"){ }

  /** Introduce this to the global factory */
  static void
  introduceSelf();

  /** Information for a particular EXE watch */
  class EXEInfo : public WatchInfo {
public:
    friend EXEWatcher;

    EXEInfo(IOListener * l, const std::string& params) : WatchInfo(l), myParams(params)
    {
      myConnected   = false;
      myCoutPipe[0] = myCoutPipe[1] = -1;
      myCerrPipe[0] = myCoutPipe[1] = -1;
    }

    /** Spawn and connect to the exe */
    bool
    connect();

    /** Create the events to be processed later */
    virtual void
    createEvents(WatcherType * w) override;
protected:

    /** Pipe for cout for exe */
    int myCoutPipe[2];

    /** Pipe for cerr for exe */
    int myCerrPipe[2];

    /** My file actions */
    posix_spawn_file_actions_t myFA;

    /** Are we connected to the EXE? */
    bool myConnected;

    /** Child pid */
    pid_t myPid;

    /** Parameters used for calling the executable */
    std::string myParams;
  };

  /** Attach a pulse to web page for a given listener to us */
  virtual bool
  attach(const std::string& dirname, bool realtime, bool archive, IOListener *) override;

  /** Get some events.  Depending on watcher this can be a global process or
   * passed on the individual infos */
  virtual void
  getEvents() override;

  /** Destroy us */
  virtual ~EXEWatcher(){ }
};
}
