#include "rTimeDuration.h"

#include "rTime.h"
#include "rError.h"
#include "rArith.h"

#include <cmath>
#include <iostream>

namespace rapio {
Time
TimeDuration::operator + (const Time& t) const
{
  return (t + (*this));
}

TimeDuration
operator * (double s, const TimeDuration& t)
{
  return TimeDuration(s * t.milliseconds());
}

std::ostream&
operator << (std::ostream& os, const TimeDuration& t)
{
  return (os << "[" << t.milliseconds() / 1000.0 << " s]");
}
}
