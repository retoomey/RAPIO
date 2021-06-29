#pragma once

#include <rUtility.h>

#include <rError.h>
#include <rTimeDuration.h>

/** Using boost timer class now internally */
#include <boost/timer/timer.hpp>

namespace rapio {
class ProcessTimer;

/** Allow aggregation of timers to add up accumulated multiple timer calls.  Memory is saved as min and maximum usage.
 * @author Robert Toomey
 */
class ProcessTimerSum : public Utility {
public:
  /** Create a new process timer sum */
  ProcessTimerSum() : myCounter(0){ };

  /** Operator << write out a ProcessTimerSum */
  friend std::ostream&
  operator << (std::ostream& os, const ProcessTimerSum&);

  /** Add a given process timer at current timer state to our summery information */
  void
  add(const ProcessTimer& timer);

  /** Get number of ProcessTimers added */
  size_t
  getCount(){ return myCounter; }

protected:

  /** Total process timers we have added */
  size_t myCounter;

  /** Total wall time for all added ProcessTimers */
  TimeDuration myWall;

  /** Total user time for all added ProcessTimers */
  TimeDuration myUser;

  /** Total user time for all added ProcessTimers */
  TimeDuration mySystem;
};

/** Output a ProcessTimerSum */
std::ostream      &
operator << (std::ostream&,
  const ProcessTimerSum&);

/**
 * The ProcessTimer object will LogInfo the msg string along with the
 * time and memory information when destroyed, iff the msg is not blank
 */
class ProcessTimer : public Utility {
public:
  friend ProcessTimerSum;

  /** Process time with no message */
  ProcessTimer() : ProcessTimer(""){ };

  /** Operator << write out a ProcessTimer */
  friend std::ostream&
  operator << (std::ostream& os, const ProcessTimer&);

  /** Process time with a final message on destruction */
  ProcessTimer(const std::string& message);

  /** Restart the timer.  ProcessTimer starts on actual construction,
   * this can be used to reset later if needed */
  void
  reset();

  /**
   * The time this ran based on clock on the wall
   */
  TimeDuration
  getWallTime();

  /**
   * The CPU time spent by the user
   */
  TimeDuration
  getUserTime();

  /**
   * The CPU time spent by the system servicing user requests
   */
  TimeDuration
  getSystemTime();

  /** The CPU time used by this process, its children and by
   *  the system on behalf of this process since the
   *  creation of this object.
   */
  TimeDuration
  getCPUTime();

  /** Destroy a process timer, possibly printing final timer state */
  ~ProcessTimer();

protected:

  /** The message to print on destruction */
  std::string myMsg;

  /** Boost internal timer */
  boost::timer::cpu_timer myTimer;
};

/** Output a process timer */
std::ostream      &
operator << (std::ostream&,
  const ProcessTimer&);
}
