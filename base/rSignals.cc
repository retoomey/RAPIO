#include "rSignals.h"
#include "rError.h"
#include "rEventLoop.h"

#include <sys/resource.h> // rlimit, etc
#include <iostream>
#include <csignal>

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace rapio;

static bool enabledStackTrace = false;

/** Designed by nobar on stackoverflow,
 * doesn't work with valgrind or when running within gdb */
void
Signals::printTrace()
{
  char pid_buf[30];

  sprintf(pid_buf, "%d", getpid());
  char name_buf[512];

  name_buf[readlink("/proc/self/exe", name_buf, 511)] = 0;
  int child_pid = fork();

  if (!child_pid) {
    dup2(2, 1); // redirect output to stderr
    fprintf(stdout, "stack trace for %s pid=%s\n", name_buf, pid_buf);
    execlp("gdb", "gdb", "--batch", "-n", "-ex", "thread", "-ex", "bt", name_buf, pid_buf, NULL);
    abort(); /* If gdb failed to start */
  } else {
    waitpid(child_pid, NULL, 0);
  }
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
  int exitcode = 1;

  // Our safe print info on signal. It's not recommended
  // to print from signals but I find this so handy in operations
  switch (signum) {
      case SIGTERM: {
        PRINTSIGNAL("SIGTERM", " received.");
        exitcode = 0;
      }
      break;
      case SIGSEGV: {
        PRINTSIGNAL("SIGSEGV", " received.");
        exitcode = 139;
      }
      break;
      case SIGINT: {
        PRINTSIGNAL("SIGINT", " received.");
        exitcode = 130;
      }
      break;
      case SIGILL: {
        PRINTSIGNAL("SIGILL", " received.");
        exitcode = 132;
      }
      break;
      case SIGABRT: {
        PRINTSIGNAL("SIGABRT", " received.");
        exitcode = 134;
      }
      break;
      case SIGFPE: {
        PRINTSIGNAL("SIGFPE", " Bad math, divide by zero?");
        exitcode = 136;
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
    printTrace();
  }
  // Frustrating, still not working in docker, maybe because I'm multithreaded?
  // possibly I'm building container entry point incorrect..but the signal is lost
  // and algorithm refuses to die.  Exit appears to work correctly so we'll use it for now.
  // kill(getpid(), signum);
  //
  // I think the issue was that signals are caught by random threads, not always
  // the main thread.  So instead we tell the main loop to die
  EventLoop::exit(exitcode);
} // Signals::handleSignal

void
Signals::initialize(bool enableStackTrace, bool wantCoreDumps)
{
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

  if (enable) {
    std::system("ulimit -c unlimited"); // try unlimiting us
  }

  // Get current limits...
  struct rlimit corelimit;
  int err = getrlimit(RLIMIT_CORE, &corelimit);

  // Note:  There's a setrlimit bug where you can lower core limits
  // but not raise them back up again, even if it's under limits.conf
  // Also you can't set them above the limits.conf settings
  // See if core limits are not zero...
  if ((err != -1) && ((corelimit.rlim_cur != want) || (corelimit.rlim_max != want))) {
    // LogInfo("Core limits (soft):" << corelimit.rlim_cur << " (hard): " <<
    // corelimit.rlim_max << "\n");

    // ... if so, try to make them 'on' or 'off'
    corelimit.rlim_cur = corelimit.rlim_max = want;
    setrlimit(RLIMIT_CORE, &corelimit);

    // ... and then check the new setting 'took'
    err = getrlimit(RLIMIT_CORE, &corelimit);

    if ((err != -1) && (corelimit.rlim_cur == want) && (corelimit.rlim_max == want)) {
      LogInfo("Core limits set (soft):" << corelimit.rlim_cur << " (hard): "
                                        << corelimit.rlim_max << "\n");
    } else {
      LogSevere("Unable to set core limits\n");
      LogSevere("Core limits set (soft):" << corelimit.rlim_cur << " (hard): "
                                          << corelimit.rlim_max << "\n");
    }
  }
} // Signals::setupCoreDumps
