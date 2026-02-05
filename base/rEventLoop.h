#pragma once
#include <memory>
#include <vector>
#include <rBOOST.h>

BOOST_WRAP_PUSH
#include <boost/asio.hpp>
BOOST_WRAP_POP

#include <iostream>
#include <thread>

namespace rapio {
class EventHandler; // Forward declaration

/** Main event loop of the running application.
 * Handles timer EventHandlers for firing actions
 * that occur in the main thread (typically).
 *
 * @author Robert Toomey
 * @ingroup rapio_event
 * @brief Runs the main loop of the application
 */
class EventLoop {
public:

  /** The global io_context singleton */
  static boost::asio::io_context&
  io_context()
  {
    static boost::asio::io_context ctx;

    return ctx;
  }

  /** Add EventHandler to the main loop */
  static void
  addEventHandler(std::shared_ptr<EventHandler> t);

  /** Starts timers/handlers and enters the blocking run loop */
  static void
  doEventLoop();

  /** Exit/shutdown with a exit code
   * Stores the code and stops the ASIO loop, causing doEventLoop to return.
   */
  static void
  exit(int theExitCode)
  {
    exitCode = theExitCode;
    io_context().stop();
  }

  /** Get the exit code we exited on */
  static int getExitCode(){ return exitCode; }

  // FIXME: Do we still need this?  This is an actual
  // separate thread (a webserver has to be)
  // There's probably a more BOOST way to do this

  /** Kept for rWebServer compatibility */
  static std::vector<std::thread> theThreads;

private:

  /** Exit code to use  */
  static int exitCode;

  /** Timer/heartbeats in main loop */
  static std::vector<std::shared_ptr<EventHandler> > myEventHandlers;
};
}
