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

// vector<EventCallback *> EventLoop::allEventActions;
// vector<std::shared_ptr<EventHandler> > EventLoop::myEventHandlers;
vector<std::shared_ptr<EventTimer> > EventLoop::myTimers;

size_t EventLoop::LOOP_IDLE_MS  = 1000; // second
size_t EventLoop::MAX_CALLBACKS = 1000;

void
EventLoop::doEventLoop()
{
  //  URL loc("/home/dyolf/test.xml");
  //  IOXML::readXMLDocument(loc);
  //  exit(1);
  // LogSevere("OK DOING TIME TEST\n");
  // Test::doTests();
  // exit(1);

  /** Make sure event queue size is initialized */
  //  std::shared_ptr<EventTimer> aTimer(new EventTimer(10000));
  //  std::shared_ptr<EventTimer> bTimer(new EventTimer(500));
  //  myTimers.push_back(aTimer);
  //  myTimers.push_back(bTimer);

  // This is our event loop.  We use a poll/delay loop
  LogInfo("Starting MAIN loop...." << myTimers.size() << "\n");
  for (auto& i:myTimers) {
    LogSevere("TIMER: " << i->myName << "\n");
  }

  auto start = std::chrono::high_resolution_clock::now();

  for (auto& i:myTimers) {
    i->start(start);
  }

  while (1) {
    // What time at start of loop?  We'll update timers
    start = std::chrono::high_resolution_clock::now();

    /*
     *  // Grab current events from our providers
     *  // Humm: if this is huge we're just spinning our wheels
     *  // once we get past max callbacks
     *  for (auto& i:myEventHandlers) {
     *    i->addCallbacks(allEventActions);
     *  }
     *  const size_t queue = allEventActions.size();
     *
     *  // Sleep if no events to process
     *  if ((queue == 0) && (LOOP_IDLE_MS > 0)) {
     */
    // Check if any timer ready during our 'idle'.  If so,
    // we short circuit our idle to min timer ready time.
    // We might not need this level of accuracy for timers

    /*      double waitMS = LOOP_IDLE_MS;
     *    auto   now    = std::chrono::high_resolution_clock::now();
     *
     *    for (auto& i:myTimers) {
     *      double readyMS = i->readyInMS(now);
     * //LogSevere("Ready " << i << " " << readyMS << " " << i->myDelayMS << "\n");
     *      if (readyMS < waitMS) {
     *        // LogDebug("Later calculation? " << readyMS << "\n");
     *        waitMS = readyMS;
     *      }
     *    }
     *
     *    // How does sleep work with timers though.. we should be
     *    // checking them...Ahh we check left time on them and sleep less
     *    if (waitMS >= 0.0) {       // Otherwise we have a timer ready
     *      size_t w = ceil(waitMS); // positive round up.  NEED THIS or
     *                               // we don't wait long enough and loop
     *                               // goes too fast
     *                               // Example wait 899.88 waits 899,
     *                               // Timer still has .88 so doesn't fire
     *      // LogDebug("Waiting " << waitMS << " " << w << "\n");
     *      std::this_thread::sleep_for(std::chrono::milliseconds(w));
     *    }
     */

    /*
     *  } else if (queue > MAX_CALLBACKS) {
     *    LogSevere(
     *      "LAG SKIPPING: " << queue << " records in the queue.\n" <<
     *        "  This is greater than " << MAX_CALLBACKS << "\n");
     *    allEventActions.clear();
     *  } else {
     *    LogDebug(queue << " event actions to process.\n");
     *
     *    // we'll make a copy of current action list, so that we don't
     *    // get into circular loops trying to modify allEventActions by, for
     *    // example, adding a new LBEventLoop ...
     *    // How can the above happen?  We do a PULL each time before processing..
     *    // nothing should be changing this list.
     *    vector<EventCallback *> temp;
     *    std::swap(allEventActions, temp);
     *
     *    for (auto& i: temp) {
     *      i->actionPerformed();
     *      delete i;
     *    }
     *  }
     */

    // Handle timers.  Each one gets a chance.  Since we don't use
    // interrupts it's up to me to write classes that yield
    //    bool hitOne = false;
    for (auto& i:myTimers) {
      // Keep time fetches IN loop, timers take time to do stuff
      auto atNow  = std::chrono::high_resolution_clock::now();
      double here = i->readyInMS(atNow);

      // LogDebug("----> at now " << here << "\n");

      if (here <= 0.0) {
        //        hitOne = true;
        i->action(); // takes time
        auto newNow = std::chrono::high_resolution_clock::now();
        i->start(newNow); // reset very close to now, or then... ;)
      }
    }

    // End of loop
    //    auto end =
    //      std::chrono::high_resolution_clock::now();
    //    std::chrono::duration<double, std::milli> elapsed = end - start;
    //    LogSevere("Main loop heartbeat: " << elapsed.count() <<
    //              " ms hit: " << hitOne << "\n");
  }
} // EventLoop::doEventLoop
