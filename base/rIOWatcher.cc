#include "rIOWatcher.h"

#include "rEventLoop.h"
#include "rFactory.h"

// Built in subclasses
#include "rFAMWatcher.h"
#include "rWebIndexWatcher.h"
#include "rDirWatcher.h"

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
  DirWatcher::introduceSelf();
}

std::shared_ptr<WatcherType>
IOWatcher::getIOWatcher(const std::string& name)
{
  return (Factory<WatcherType>::get(name, "IOWatcher"));
}
