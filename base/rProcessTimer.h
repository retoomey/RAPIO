#pragma once

#include <rError.h>
#include <rTimeDuration.h>
#include <rColorTerm.h>

/** Using boost timer class now internally */
#include <rBOOST.h>

BOOST_WRAP_PUSH
#include <boost/timer/timer.hpp>
BOOST_WRAP_POP

namespace rapio {
class ProcessTimer;

/** Allow aggregation of timers to add up accumulated multiple timer calls.
 *
 * @code
 * ProcessTimerSum sum("Exact sum timers\n");
 * for(size_t i = 0; i<10; ++i){
 *   ProcessTimer timer("Time this action\n");
 *   // Do stuff that takes CPU and/or increases RAM
 *   fLogInfo("{}", timer);
 *   sum.add(timer);
 * }
 * fLogInfo("{}", sum);
 * @endcode
 *
 * @author Robert Toomey
 * @ingroup rapio_utility
 * @brief Handle grouping a bunch of ProcessTimer.
 *
 */
class ProcessTimerSum {
public:
  /** Create a new process timer sum */
  ProcessTimerSum(const std::string& message = "");

  /** Add a given process timer at current timer state to our summery information */
  void
  add(const ProcessTimer& timer);

  /** Get number of ProcessTimers added */
  size_t
  getCount(){ return myCounter; }

  /** Operator << write out a ProcessTimerSum */
  friend std::ostream&
  operator << (std::ostream& os, const ProcessTimerSum&);

protected:

  /** Total process timers we have added */
  size_t myCounter;

  /** Total wall time for all added ProcessTimers */
  TimeDuration myWall;

  /** Total user time for all added ProcessTimers */
  TimeDuration myUser;

  /** Total user time for all added ProcessTimers */
  TimeDuration mySystem;

  /** Total CPU percentages for N ProcessTimers */
  float myCPU;

  /** The message to print on destruction */
  std::string myMsg;

  /** Newline given at end of the message? */
  bool myHaveNewline;

  /** Resident set size memory in kilobytes at start or post reset */
  double myRSS_KB;

  /** Virtual memory in kilobytes at start or post reset */
  double myVM_KB;
};

/** Output a ProcessTimerSum */
std::ostream      &
operator << (std::ostream&,
  const ProcessTimerSum&);

/**
 * The ProcessTimer object will fLogInfo the msg string along with the
 * time and memory information when destroyed, iff the msg is not blank
 *
 * @code
 * ProcessTimer timer("Time this action\n");
 * // Do stuff that takes CPU and/or increases RAM
 * fLogInfo("{}", timer);
 * @endcode
 *
 * @author Robert Toomey
 * @ingroup rapio_utility
 * @brief Time a process.
 */
class ProcessTimer {
public:
  friend ProcessTimerSum;

  /** Create a process timer keeping track of time and memory */
  ProcessTimer(const std::string& message = "");

  /** Operator << write out a ProcessTimer */
  friend std::ostream&
  operator << (std::ostream& os, const ProcessTimer&);

  /** Restart the timer.  ProcessTimer starts on actual construction,
   * this can be used to reset later if needed */
  void
  reset();

  /** Get virtual memory in kilobytes used/freed since beginning of timer */
  double
  getVirtualKB();

  /** Get resident memory in kilobytes used/freed since beginning of timer */
  double
  getResidentKB();

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

protected:

  /** The message to print on destruction */
  std::string myMsg;

  /** Newline given at end of the message? */
  bool myHaveNewline;

  /** Boost internal timer */
  boost::timer::cpu_timer myTimer;

  /** Resident set size memory in kilobytes at start or post reset */
  double myRSS_KB;

  /** Virtual memory in kilobytes at start or post reset */
  double myVM_KB;
};

/** Output a process timer */
std::ostream      &
operator << (std::ostream&,
  const ProcessTimer&);
}

/** Format library support, allows fLogInfo("ProcessTimer {}", processTimer) */
template <>
struct fmt::formatter<rapio::ProcessTimer> {
  constexpr auto parse(format_parse_context& ctx){ return ctx.begin(); }

  template <typename FormatContext>
  auto
  format(const rapio::ProcessTimer& pt, FormatContext& ctx) const
  {
    std::stringstream ss; // FIXME: We shouldn't user operator, it should use us

    ss << pt;
    return fmt::format_to(ctx.out(), "{}", ss.str());
  }
};

/** Format library support, allows fLogInfo("ProcessTimerSum {}", processTimerSum) */
template <>
struct fmt::formatter<rapio::ProcessTimerSum> {
  constexpr auto parse(format_parse_context& ctx){ return ctx.begin(); }

  template <typename FormatContext>
  auto
  format(const rapio::ProcessTimerSum& pt, FormatContext& ctx) const
  {
    std::stringstream ss; // FIXME: We shouldn't user operator, it should use us

    ss << pt;
    return fmt::format_to(ctx.out(), "{}", ss.str());
  }
};
