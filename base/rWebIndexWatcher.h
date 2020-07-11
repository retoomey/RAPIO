#pragma once

#include <rIOWatcher.h>
#include <rEventLoop.h>
#include <rEventTimer.h>
#include <rURL.h>

namespace rapio {
/** Handles collections of web indexes and pulsing time to them
 * as a group.  More of a timer callback for a group than a watcher,
 * but this feels to match in spirit.
 *
 * @author Robert Toomey
 */
class WebIndexWatcher : public WatcherType {
public:
  WebIndexWatcher() : WatcherType(5000, 2, "Web index event handler"){ }

  /** Introduce this to the global factory */
  static void
  introduceSelf();

  class WebInfo : public WatchInfo {
public:
    URL myURL;

    WebInfo(IOListener * l, const std::string& dir)
      : myURL(dir)
    {
      myListener = l;
    }
  };

  /** Attach a pulse to web page for a given listener to us */
  virtual bool
  attach(const std::string& dirname, IOListener *) override;

  /** Action to take on timer pulse */
  virtual void
  action() override;

  /** Destroy us */
  virtual ~WebIndexWatcher(){ }
};
}
