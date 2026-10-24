#include "rSignals.h"
#include "rError.h"
#include "rEventLoop.h"
#include "rOS.h"
#include "rBOOST.h"
BOOST_WRAP_PUSH
#include <boost/stacktrace.hpp>
BOOST_WRAP_POP

#include <sys/resource.h> // rlimit, etc
#include <iostream>
#include <csignal>

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace rapio;

static bool enabledStackTrace      = false;
bool Signals::myCoreDumpsRequested = false;

void
Signals::printTrace(const std::string& reason)
{
  // Note: Boost allows various stacktrace levels.
  // For full listing need stacktrace_addr2line plus RelWithDebInfo
  //
  std::cerr << "\n[RAPIO " << reason << "] Build: " << OS::getBuildInfo() << "\n"
            << "Stack trace:\n"
            <<
    boost::stacktrace::stacktrace() << "\n\n";
}

/*
 * SIGTERM	termination request, sent to the program
 * SIGSEGV	invalid memory access (segmentation fault)
 * SIGINT	external interrupt, usually initiated by the user
 * SIGILL	invalid program image, such as invalid instruction
 * SIGABRT	abnormal termination condition, as is e.g. initiated by abort()
 * SIGFPE	erroneous arithmetic operation such as divide by zero
 */

#define PRINTSIGNAL(X, Y)                              \
  do {                                                  \
    char signal[]      = X;                                  \
    char msg[]         = Y;                                     \
    const char stock[] = " Terminating algorithm.\n";    \
    write(STDOUT_FILENO, signal, sizeof(signal));       \
    write(STDOUT_FILENO, msg, sizeof(msg));             \
    write(STDOUT_FILENO, stock, sizeof(stock));         \
  } while (0)

void
Signals::handleSignal(int signum)
{
  // The exit code magic numbers are out on the web.  We'll try
  // to send the correct ones, normally it's just 1 for error 0 for success
  int exitcode       = 1;
  std::string reason = "CRASH";

  // Our safe print info on signal. It's not recommended
  // to print from signals but I find this so handy in operations
  switch (signum) {
      case SIGTERM: {
        PRINTSIGNAL("SIGTERM", " received.");
        exitcode = 0;
        reason   = "TERMINATED";
      }
      break;
      case SIGSEGV: {
        PRINTSIGNAL("SIGSEGV", " received.");
        exitcode = 139;
        reason   = "SEGFAULT";
      }
      break;
      case SIGINT: {
        PRINTSIGNAL("SIGINT", " received.");
        exitcode = 130;
        reason   = "INTERRUPT";
      }
      break;
      case SIGILL: {
        PRINTSIGNAL("SIGILL", " received.");
        exitcode = 132;
        reason   = "ILLEGAL INSTRUCTION";
      }
      break;
      case SIGABRT: {
        PRINTSIGNAL("SIGABRT", " received.");
        exitcode = 134;
        reason   = "ABORT";
      }
      break;
      case SIGFPE: {
        PRINTSIGNAL("SIGFPE", " Bad math, divide by zero?");
        exitcode = 136;
        reason   = "MATH ERROR";
      }
      break;
      default: {
        PRINTSIGNAL("UNKNOWN", " signal number.");
        exitcode = 2;
      }
      break;
  }

  // Found stackoverflow recommendations on removing the signal handlers
  // and then self calling kill.  We also want this behavior because
  // in containers we don't necessarily have a shell to handle SIGINT
  // properly for us.
  signal(SIGTERM, SIG_DFL);
  signal(SIGSEGV, SIG_DFL);
  signal(SIGINT, SIG_DFL);
  signal(SIGILL, SIG_DFL);
  signal(SIGABRT, SIG_DFL);
  signal(SIGFPE, SIG_DFL);

  // Ok try to avoid infinite loop on any possibility of printTrace signaling here
  // exit itself is smart enough for double call
  if (enabledStackTrace) {
    enabledStackTrace = false;
    printTrace(reason);
  }

  // -------------------------------------------------------------------------
  // In a multithreaded environment (like Docker containers running this app),
  // POSIX signals like SIGINT or SIGTERM are delivered to whichever thread
  // happens to be available.
  //
  // If a background thread catches the signal and calls standard std::exit(),
  // C++ attempts to cleanly shut down the process (flushing I/O, running
  // static destructors). BUT std::exit() is NOT async-signal-safe! If the
  // main thread was holding a mutex or writing to a log when the signal hit,
  // the shutdown sequence deadlocks, the app freezes, and Docker eventually
  // has to resort to a hard SIGKILL.
  //
  // To fix this, we first flag the ASIO loop to stop (just in case).
  EventLoop::exit(exitcode);

  // Then, we use std::_Exit() (capital E) to ask the OS to drop the guillotine
  // immediately. This bypasses the C++ shutdown sequence entirely, preventing
  // deadlocks and guaranteeing an instant, safe exit from any thread.
  // -------------------------------------------------------------------------
  std::_Exit(exitcode);
} // Signals::handleSignal

void
Signals::initialize(bool enableStackTrace, bool wantCoreDumps)
{
  myCoreDumpsRequested = wantCoreDumps;
  auto aSignal = handleSignal;

  enabledStackTrace = enableStackTrace;

  // Set or remove signal handling depending on flags
  signal(SIGTERM, aSignal);
  signal(SIGSEGV, aSignal);
  signal(SIGINT, aSignal);
  signal(SIGILL, aSignal);
  signal(SIGABRT, aSignal);
  signal(SIGFPE, aSignal);

  // Disable core dumps by setting to zero size
  setupCoreDumps(wantCoreDumps);
}

void
Signals::setupCoreDumps(bool enable)
{
  // Remember that /etc/security/limits.conf limit our ability to set stuff
  // unless we're root
  const auto want = enable ? RLIM_INFINITY : 0;

  // Get current limits...
  struct rlimit corelimit;
  int err = getrlimit(RLIMIT_CORE, &corelimit);

  // Note:  There's a setrlimit bug where you can lower core limits
  // but not raise them back up again, even if it's under limits.conf
  // Also you can't set them above the limits.conf settings
  // See if core limits are not zero...
  if (err != -1) {
    // ... if so, try to make them 'on' or 'off'
    corelimit.rlim_cur = corelimit.rlim_max = want;
    setrlimit(RLIMIT_CORE, &corelimit);
  }
} // Signals::setupCoreDumps

std::string
Signals::getCoreStatusString()
{
  // ... and then check the new setting 'took'
  struct rlimit corelimit;

  if (getrlimit(RLIMIT_CORE, &corelimit) == -1) {
    return "Core limits: Failed to query system limits.";
  }

  const auto want = myCoreDumpsRequested ? RLIM_INFINITY : 0;

  // Check if the system actually gave us what we wanted
  if (corelimit.rlim_cur == want) {
    return fmt::format("Core limits set (soft): {} (hard): {}",
             corelimit.rlim_cur, corelimit.rlim_max);
  } else {
    return fmt::format("Core limit mismatch! Wanted {}, but system is {}",
             want, corelimit.rlim_cur);
  }
}
