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
  WebIndexWatcher() : WatcherType(5000, "Web index event handler"){ }

  static void
  introduceSelf();

  class WebInfo : public IO { // FIXME: could subclass one from watcher
public:
    IOListener * myListener;
    URL myURL;

    WebInfo(IOListener * l, const std::string& dir)
      : myListener(l), myURL(dir){ }
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
  virtual ~WebIndexWatcher(){ }

protected:

  /** The list of watches we currently have */
  std::vector<WebInfo> myWatches;

private:

  /** Get new WEB events into the queue */
  void
  getEvents();
};
}
