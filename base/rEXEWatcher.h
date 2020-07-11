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
  EXEWatcher() : WatcherType(100, 2, "EXE Watcher"){ }

  /** Introduce this to the global factory */
  static void
  introduceSelf();

  /** Information for a particular EXE watch */
  class EXEInfo : public WatchInfo {
public:

    /** Pipe for cout for exe */
    int myCoutPipe[2];

    /** Pipe for cerr for exe */
    int myCerrPipe[2];

    /** My file actions */
    posix_spawn_file_actions_t myFA;

    /** Are we connected to the EXE? */
    bool myConnected;

    EXEInfo(IOListener * l, const std::string& params)
    //   : myParams(params)
    {
      myListener  = l;
      myParams    = params;
      myConnected = false;
    }

    /** Spawn and connect to the exe */
    bool
    connect();

    /** Create the events to be processed later */
    virtual void
    createEvents(WatcherType * w) override;
protected:
    /** Child pid */
    pid_t myPid;

    std::string myParams;
  };

  /** Attach a pulse to web page for a given listener to us */
  virtual bool
  attach(const std::string& dirname, IOListener *) override;

  /** Get some events.  Depending on watcher this can be a global process or
   * passed on the individual infos */
  virtual void
  getEvents() override;

  /** Destroy us */
  virtual ~EXEWatcher(){ }
};
}
