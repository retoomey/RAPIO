#include "rIOWatcher.h"

#include "rEventLoop.h"
#include "rFactory.h"

// Built in subclasses
#include "rFAMWatcher.h"
#include "rWebIndexWatcher.h"
#include "rDirWatcher.h"
#include "rEXEWatcher.h"

#include <string>
#include <memory>

using namespace rapio;
using namespace std;

void
IOWatcher::introduce(const string & protocol,
  std::shared_ptr<WatcherType>    factory)
{
  Factory<WatcherType>::introduce(protocol, factory);
  EventLoop::addEventHandler(factory);
}

void
IOWatcher::introduceSelf()
{
  FAMWatcher::introduceSelf();
  WebIndexWatcher::introduceSelf();
  DirWatcher::introduceSelf();
  EXEWatcher::introduceSelf();
}

std::shared_ptr<WatcherType>
IOWatcher::getIOWatcher(const std::string& name)
{
  return (Factory<WatcherType>::get(name, "IOWatcher"));
}

void
WatcherType::action()
{
  // Notify watches of our poll
  for (auto&w:myWatches) {
    w->handlePoll();
  }

  // Humm should we get events first or process events first to
  // clear old ones?

  // Add some new events if able
  if (!queueIsThrottled()) {
    getEvents();
  }

  // Process a few if able
  processEvents();
}

bool
WatcherType::queueIsThrottled()
{
  // If queue is full do we wait or drop oldest...wait when full
  // we don't poll for events.
  if (myEvents.size() == myMaxQueueSize) {
    if (myWaitWhenQueueFull) {
      LogSevere("Queue is full..waiting on creating new events.\n");
      return true;
    } else {
      LogInfo("Queue size is over " << myMaxQueueSize << ", system is probably lagging.\n");
    }
  }
  return false;
}

void
WatcherType::processEvents()
{
  // Do a few actions and don't hog the system.
  // Typically the longer the required poll time, the more events
  // you can process here.
  // FIXME: Maybe use a process timer here to determine
  for (size_t i = 0; i < myProcessCount; ++i) {
    if (!myEvents.empty()) {
      auto e = myEvents.front();
      myEvents.pop();
      e.handleEvent();
    }
  }
}

void
WatcherType::createNewEvents()
{
  // Default lets the watches create events
  for (auto& w:myWatches) {
    w->createEvents(this);
  }
}

void
WatcherType::detach(IOListener * l)
{
  for (auto& w:myWatches) {
    // If listener at i matches us...
    if (w->myListener == l) {
      bool deleteit = w->handleDetach(this);
      if (deleteit) {
        w->myListener = nullptr;
      }
    }
  }

  // Lamba erase all zeroed listeners
  myWatches.erase(
    std::remove_if(myWatches.begin(), myWatches.end(),
    [](const std::shared_ptr<WatchInfo> &w){
    return (w->myListener == nullptr);
  }),
    myWatches.end());
}

void
WatchEvent::handleEvent()
{
  myListener->handleNewEvent(this);
}
