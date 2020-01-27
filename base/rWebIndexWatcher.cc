#include "rWebIndexWatcher.h"
#include "rWebIndex.h"

#include "rError.h" // for LogInfo()

using namespace rapio;

void
WebIndexWatcher::action()
{
  getEvents();
}

void
WebIndexWatcher::getEvents()
{
  // Let web indexes push newest records into the record queue directly
  // We don't need to store anything extra ourselves like FAMWatcher, for example
  for (auto&w:myWebWatches) {
    w->myListener->handlePoll();
  }
}

bool
WebIndexWatcher::attach(const std::string & dirname,
  IOListener *                            l)
{
  std::shared_ptr<WebInfo> newWatch = std::make_shared<WebInfo>(l, dirname);
  myWebWatches.push_back(newWatch);
  return true;
}

void
WebIndexWatcher::detach(IOListener * l)
{
  for (auto& w:myWebWatches) {
    // If listener at i matches us...
    if (w->myListener == l) {
      w->myListener = nullptr;
    }
  }

  // Lamba erase all zeroed listeners
  myWebWatches.erase(
    std::remove_if(myWebWatches.begin(), myWebWatches.end(),
    [](const std::shared_ptr<WebInfo> &w){
    return (w->myListener == nullptr);
  }),
    myWebWatches.end());
}

void
WebIndexWatcher::introduceSelf()
{
  std::shared_ptr<WebIndexWatcher> io = std::make_shared<WebIndexWatcher>();
  IOWatcher::introduce("web", io);
}
