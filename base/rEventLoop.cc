#include "rEventLoop.h"

#include "rEventTimer.h"
#include "rOS.h"
#include "rError.h"
#include "rTime.h"

#include "rTest.h"
#include "rIOXML.h"
#include "rURL.h"

#include <cassert>
#include <cmath>
#include <thread>

using namespace rapio;
using namespace std;

using namespace std::chrono;

vector<std::shared_ptr<EventTimer> > EventLoop::myTimers;

void
EventLoop::doEventLoop()
{
  // This is our event loop.  We use a poll/delay loop
  LogInfo("Starting MAIN loop...." << myTimers.size() << "\n");
  // for (auto& i:myTimers) {
  //  LogSevere("TIMER: " << i->myName << "\n");
  // }

  // Start all registered timer objects
  auto start = std::chrono::high_resolution_clock::now();
  for (auto& i:myTimers) {
    i->start(start);
  }

  while (1) {
    // What time at start of loop?  We'll update timers
    start = std::chrono::high_resolution_clock::now();

    // Check if any timer ready during our 'idle'.  If so,
    // we short circuit our idle to min timer ready time.
    // We might not need this level of accuracy for timers
    // Handle timers.  Each one gets a chance.  Since we don't use
    // interrupts it's up to me to write classes that yield
    for (auto& i:myTimers) {
      // Keep time fetches IN loop, timers take time to do stuff
      auto atNow  = std::chrono::high_resolution_clock::now();
      double here = i->readyInMS(atNow);
      if (here <= 0.0) {
        i->action(); // takes time
        auto newNow = std::chrono::high_resolution_clock::now();
        i->start(newNow); // reset very close to now, or then... ;)
      }
    }
  }
} // EventLoop::doEventLoop
