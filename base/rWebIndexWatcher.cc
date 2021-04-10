#include "rWebIndexWatcher.h"
#include "rWebIndex.h"

#include "rError.h" // for LogInfo()

using namespace rapio;

/** Default constant for a web watcher */
const std::string WebIndexWatcher::WEB_WATCH = "web";

void
WebIndexWatcher::action()
{
  // We don't use queue for now, web index and the last flags are
  // designed to stop spamming.
  // Let web indexes push newest records into the record queue directly
  // We don't need to store anything extra ourselves like FAMWatcher, for example
  // FIXME: Maybe generate events here too
  for (auto&w:myWatches) {
    // w->myListener->handlePoll();
    w->handlePoll();
  }
}

bool
WebIndexWatcher::attach(const std::string & dirname,
  bool                                    realtime,
  bool                                    archive,
  IOListener *                            l)
{
  std::shared_ptr<WebInfo> newWatch = std::make_shared<WebInfo>(l, dirname);
  myWatches.push_back(newWatch);
  return true;
}

void
WebIndexWatcher::introduceSelf()
{
  std::shared_ptr<WebIndexWatcher> io = std::make_shared<WebIndexWatcher>();
  IOWatcher::introduce(WEB_WATCH, io);
}
