#include "rSignals.h"
#include "rError.h"

#include <sys/resource.h> // rlimit, etc
#include <iostream>
#include <csignal>

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace rapio;

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
void
Signals::handleSignal(int signum)
{
  switch (signum) {
      case SIGTERM: std::cout << "\nTERMinated " << signum << "\n";
        break;
      case SIGSEGV: std::cout << "\nEGC: Invalid memory " << signum << "\n";
        break;
      case SIGINT: std::cout << "\nINT: External interrupt " << signum << "\n";
        break;
      case SIGILL: std::cout << "\nILL: Invalid program image " << signum << "\n";
        break;
      case SIGABRT: std::cout << "\nABRT: Abnormal termination " << signum << "\n";
        break;
      case SIGFPE: std::cout << "\nFPE: Bad math, divide by zero? " << signum << "\n";
        break;
      default:
        std::cout << "\nUnknown signal number " << signum << "\n";
        break;
        break;
  }
  printTrace();
  exit(signum);
}

void
Signals::initialize(bool enableStackTrace, bool wantCoreDumps)
{
  // Try the gdb backtrace on signal
  if (enableStackTrace) { // on-off maybe not enough level of control
    // Also we can get multiple signals and stack dumps.  I think
    // that is ok though.  We'll probably just turn on in the
    // verbose="debug" mode
    signal(SIGTERM, handleSignal);
    signal(SIGSEGV, handleSignal);
    signal(SIGINT, handleSignal);
    signal(SIGILL, handleSignal);
    signal(SIGABRT, handleSignal);
    signal(SIGFPE, handleSignal);
  }

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
