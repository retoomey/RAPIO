#pragma once

#include <rEventTimer.h>
#include <rTime.h>

#include <string>
#include <memory>

// Apache 2.0 license
// Think this is ok here, including license for it
extern "C" {
#include "ccronexpr.h"
}

namespace rapio {
class RAPIOProgram;

/**
 * A baby cron format, except we have seconds as well
 * This is used to fire timed events at a program for
 * accumulative non-data drive actions.
 *
 * @author Robert Toomey
 * @ingroup rapio_utility
 * @brief Handles a cron style heartbeat callback.
 */
class Heartbeat {
public:

  /** Build the heartbeat */
  Heartbeat(RAPIOProgram * prog);

  /** Parse our special crontablike language string
   * @param cronlist The string from the program parameters. */
  bool
  setCronList(const std::string& cronlist);

  /** Check for pulse and send message to program */
  void
  checkForPulse();

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

/**
 * An event timer that calls a heartbeat
 *
 * @author Robert Toomey
 * @brief EventTimer that calls a Heartbeat periodically.
 */
class HeartbeatTimer : public EventTimer {
public:
  /** Build the heartbeat */
  HeartbeatTimer(std::shared_ptr<Heartbeat>, size_t milliseconds);

  /** Do the timer action */
  virtual void
  action() override;

protected:
  /** The heartbeat we send timed calls to */
  std::shared_ptr<Heartbeat> myHeartbeat;
};
}
