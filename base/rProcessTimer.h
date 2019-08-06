#pragma once

#include <rUtility.h>

#include <rError.h>
#include <rTimeDuration.h>
#include <rTime.h>
#include <rOS.h>

#include <memory>

namespace rapio {
/**
 * The ProcessTimer object will print the string along with the
 * CPU milliseconds elapsed (and additional memory resources used)
 * since the object was constructed.
 * The printing happens when the object is destroyed.
 */
class ProcessTimer : public Utility {
public:

  /** Process time with no message */
  ProcessTimer() : ProcessTimer(""){ };

  /** Process time with a final message on destruction */
  ProcessTimer(const std::string& message);

  /**
   * The elapsed time from the creation of this object
   */
  TimeDuration
  getElapsedTime();

  /**
   * The memory currently being used by this program, in pages.
   */
  static int
  getProgramSize();

  /** The CPU time used by this process, its children and by
   *  the system on behalf of this process since the
   *  creation of this object.
   */
  TimeDuration
  getCPUTime();

  /** Print current timer state */
  void
  print();

  ~ProcessTimer();

protected:

  std::string myMsg;
  Time myStartTime;
  unsigned long myCPUTime;
};
}
