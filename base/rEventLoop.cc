#include "rEventLoop.h"

#include "rEventTimer.h"
#include "rOS.h"
#include "rError.h"
#include "rTime.h"
#include "rURL.h"

#include <cassert>
#include <cmath>
#include <thread>

using namespace rapio;
using namespace std;

using namespace std::chrono;

vector<std::shared_ptr<EventHandler> > EventLoop::myEventHandlers;

// Thread sync to main loop
// Would be nice to hide these with functions...maybe we can?
std::mutex EventLoop::theEventLock;
std::condition_variable EventLoop::theEventCheckVariable;
bool EventLoop::theReady;

// Flag to tell if we are running.  If this goes false we exit
// I'm not bothering with atomic.  We should set exitCode and
// the change isRunning if in another thread.
int EventLoop::exitCode = 0;
std::atomic<bool> EventLoop::isRunning(true);

// Global pool of threads
std::vector<std::thread> EventLoop::theThreads;

void
EventLoop::exit(int theExitCode)
{
  // Coming from a random thread here...tell the main thread
  exitCode  = theExitCode;
  isRunning = false;
}

void
EventLoop::doEventLoop()
{
  LogInfo("Starting MAIN loop with " << myEventHandlers.size() << " Event handlers.\n");
  for (auto& i:myEventHandlers) {
    LogDebug("  EventHandler: " << i->getName() << "\n");
  }

  // Create any timer threads wanted, these will fire on timers
  for (auto& i:myEventHandlers) {
    i->createTimerThread(theThreads);
  }

  // Detach the worker threads
  for (auto& t:theThreads) {
    t.detach();
  }

  // While not exited, run main loop
  try {
    while (isRunning) {
      std::unique_lock<std::mutex> lck(theEventLock);
      theEventCheckVariable.wait(lck, [] {
        return theReady;
      });

      // We're now locked, so turn off flag so another timer can trigger it again.
      // However, we can't 'do' anything while locked or any thread trying to retrigger will
      // deadlock. Copy the list of currently 'ready' timers.
      theReady = false;
      std::vector<std::shared_ptr<EventHandler> > isReady;
      for (auto& a:myEventHandlers) {
        if (a->isReady()) {
          isReady.push_back(a);
        }
      }
      lck.unlock(); // actions might turn on dataReady again from either the called threads,
                    // or others that go off while we're working

      // Now we can tell them all to do work.  Also, firing now will clear flags safely
      for (auto&a:isReady) {
        a->actionMainThread();
      }
    }
  }catch (const std::exception& e) {
    LogSevere("Main loop uncaught exception: \"" << e.what() << "\", exiting\n");
  }

  // Delete all child threads
  for (auto& t:theThreads) {
    t.~thread();
  }
} // EventLoop::doEventLoop
