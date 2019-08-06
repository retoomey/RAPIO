#include "rWebIndexWatcher.h"
#include "rWebIndex.h"

#include "rError.h" // for LogInfo()
#include "rEventLoop.h"

#include <set>
#include <cstdio>  // snprintf()
#include <cstdlib> // atol()
#include <iostream>
#include <algorithm>

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
  for (auto&w:myWatches) {
    w.myListener->handlePoll();
  }
}

bool
WebIndexWatcher::attach(const std::string & dirname,
  IOListener *                            l)
{
  myWatches.push_back(WebInfo(l, dirname));
  return true;
}

void
WebIndexWatcher::detach(IOListener * l)
{
  for (auto& w:myWatches) {
    // If listener at i matches us...
    if (w.myListener == l) {
      w.myListener = nullptr;
    }
  }

  // Lamba erase all zeroed listeners
  myWatches.erase(
    std::remove_if(myWatches.begin(), myWatches.end(),
    [](const WebInfo &w){
    return (w.myListener == nullptr);
  }),
    myWatches.end());
}

void
WebIndexWatcher::introduceSelf()
{
  std::shared_ptr<WebIndexWatcher> io(new WebIndexWatcher());
  IOWatcher::introduce("web", io);
}
