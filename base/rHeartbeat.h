#pragma once

#include <rEventTimer.h>
#include <rTime.h>

#include <string>

// Apache 2.0 license
// Think this is ok here, including license for it
#include "ccronexpr.h"

namespace rapio {
class RAPIOProgram;

/**
 * A baby cron format timer, except we have seconds as well
 * This is used to fire timed events at a program for
 * accumulative non-data drive actions.
 *
 * @author Robert Toomey
 */
class Heartbeat : public EventTimer {
public:
  /** Build the heartbeat */
  Heartbeat(RAPIOProgram * prog, size_t milliseconds);

  /** Parse our special crontablike language string
   * @param cronlist The string from the program parameters. */
  bool
  setCronList(const std::string& cronlist);

  /** Do the timer action */
  virtual void
  action() override;

protected:
  /** The program we send events to */
  RAPIOProgram * myProgram;

  /** Last pulse time (the pinned time) */
  Time myLastPulseTime;

  /** Set for first pulse of system */
  bool myFirstPulse;

  /** Is our cronlist parsed and good? */
  bool myParsed;

  /** Cron expression if successfully parsed */
  cron_expr myCronExpr;
};
}
