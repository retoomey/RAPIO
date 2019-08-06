#include "rIOWatcher.h"

// Built in subclasses
#include "rFactory.h"
#include "rFAMWatcher.h"
#include "rWebIndexWatcher.h"
#include "rEventLoop.h"

#include <string>
#include <memory>

using namespace rapio;
using namespace std;

void
IOWatcher::introduce(const string & protocol,
  std::shared_ptr<WatcherType>    factory)
{
  Factory<WatcherType>::introduce(protocol, factory);
  EventLoop::addTimer(factory);
}

void
IOWatcher::introduceSelf()
{
  FAMWatcher::introduceSelf();
  WebIndexWatcher::introduceSelf();
  // FIXME: a simple directory time based poll watcher...
}

std::shared_ptr<WatcherType>
IOWatcher::getIOWatcher(const std::string& name)
{
  return (Factory<WatcherType>::get(name, "IOWatcher"));
}
