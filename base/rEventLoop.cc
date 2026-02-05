#include "rEventLoop.h"
#include "rError.h"
#include "rEventTimer.h"

namespace rapio {
std::vector<std::shared_ptr<EventHandler> > EventLoop::myEventHandlers;
std::vector<std::thread> EventLoop::theThreads;
int EventLoop::exitCode = 0;

void
EventLoop::addEventHandler(std::shared_ptr<EventHandler> t)
{
  myEventHandlers.push_back(t);
}

void
EventLoop::doEventLoop()
{
  // 1. Detach auxiliary threads (e.g., WebServer) to run in background
  for (auto& t : theThreads) {
    if (t.joinable()) {
      t.detach();
    }
  }

  // 2. Start all registered EventHandlers
  // (Timers schedule their first tick, Queues do nothing)
  for (auto& handler : myEventHandlers) {
    handler->start();
  }

  fLogInfo("Starting MAIN loop (Boost.Asio) with {} handlers.", myEventHandlers.size());

  // 3. Create a work guard so the loop doesn't exit if the queue is temporarily empty
  auto work_guard = boost::asio::make_work_guard(io_context());

  // 4. Run the loop (Blocks here until exit() is called)
  try {
    io_context().run();
  } catch (const std::exception& e) {
    fLogSevere("Main loop uncaught exception: {}", e.what());
  }
}
}
