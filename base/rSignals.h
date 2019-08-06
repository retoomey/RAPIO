#pragma once

#include <rUtility.h>

#include <string>
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

namespace rapio {
class Signals : public Utility {
public:

  /** Initialize signal handler.
   * @param enableStackTrace Turn on to try to use gdb to dump a stack trace on signal.
   * @param wantCoreDumps Turn on to let program dump to core.  */
  static void
  initialize(bool enableStackTrace = true, bool wantCoreDumps = false);

  /** Attempt to print stack trace using gdb */
  static void
  printTrace();

private:

  /** Signal callback for handling trace */
  static void
  handleSignal(int signum);

  /** Make 100% that we disable core dumping.  For realtime systems if
   * we are crashing a lot we fill up the disks */
  static void
  setupCoreDumps(bool enable);

  Signals();
};
}
