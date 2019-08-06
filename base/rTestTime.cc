#include "rTestTime.h"

#include "rTime.h"
#include "rError.h"
#include "rArith.h"

#include "rXYZ.h"

#include <cmath>
#include <iostream>

namespace rapio {
void
TestTime::logicTest()
{
  LLH a(-34.017, 151.230, 100.0); // made up
  LLH b(-34.017, 151.230, 0.0);   // made up
  // IJK zz2 = XYZ(a)-XYZ(b);
  // std::cout << "IJK back is " << zz2.norm() << "\n";

  // std::cout << a << " and " << b << "\n";
  // check ((d1.x == 5.13828e-321) && (d1.y == -3.75769e-312) && (d1.z ==-1.18238e-314) )

  // std::cout << d1 << "\n";

  Time t = Time::SecondsSinceEpoch(100);

  check(t.getSecondsSinceEpoch() == 100);
  check(t.getSecondsSinceEpoch() == 10);
  check(t.getSecondsSinceEpoch() == 10)

  check(t.getSecondsSinceEpoch() == 100)
  check(Arith::feq(t.getFractional(), 0.0))
  t += TimeDuration::Minutes(2);
  check(t.getSecondsSinceEpoch() == 220)
  check(Arith::feq(t.getFractional(), 0.0))
  t += TimeDuration::Seconds(0.9);
  check(t.getSecondsSinceEpoch() == 220)
  check(Arith::feq(t.getFractional(), 0.9))
  t -= TimeDuration::Seconds(0.4);
  check(t.getSecondsSinceEpoch() == 220)
  check(Arith::feq(t.getFractional(), 0.5))
  t = t - TimeDuration::Seconds(1.1);
  check(t.getSecondsSinceEpoch() == 219)
  check(Arith::feq(t.getFractional(), 0.4))
  t += TimeDuration::Seconds(0.9);
  check(t.getSecondsSinceEpoch() == 220)
  check(Arith::feq(t.getFractional(), 0.3))

  Time t1(Time::SecondsSinceEpoch(100));
  Time t2(Time::SecondsSinceEpoch(101));
  check(t1 < t2)
  check(t1 <= t2)
  check(t1 != t2)
  check(!(t1 > t2))
  check(!(t1 >= t2))
  check(!(t1 == t2))
  t1 += TimeDuration::Seconds(0.5);
  t2 -= TimeDuration::Seconds(0.5);
  check(t1 == t2)
  check(t1 <= t2)
  check(t1 >= t2)
  check(!(t1 != t2))
  check(!(t1 < t2))
  check(!(t1 > t2))

  /**
  ***  ctors and accessors:
  ***  Seconds(), Minutes(), Hours(), Days()
  ***  seconds(), minutes(), hours(), days()
  **/

  check(Arith::feq(TimeDuration::Seconds(129600).seconds(), 129600))
  check(Arith::feq(TimeDuration::Seconds(129600).minutes(), 2160))
  check(Arith::feq(TimeDuration::Seconds(129600).hours(), 36))
  check(Arith::feq(TimeDuration::Seconds(129600).days(), 1.5))
  check(Arith::feq(TimeDuration::Minutes(2.0).seconds(), 120))
  check(Arith::feq(TimeDuration::Hours(2.0).seconds(), 7200))
  check(Arith::feq(TimeDuration::Days(1.5).seconds(), 129600))

  /**
  ***  operator/,
  ***  operator/=,
  ***  operator*,
  ***  operator*=,
  ***  operator+ Time,
  ***  operator+ TimeDuration,
  ***  operator+=,
  ***  operator-,
  ***  operator-=,
  **/

  TimeDuration ti = TimeDuration::Seconds(60) / 2;
  check(Arith::feq(ti.seconds(), 30))
  ti /= 2;
  check(Arith::feq(ti.seconds(), 15))
  ti *= 2;
  check(Arith::feq(ti.seconds(), 30))
  ti = ti * 2;
  check(Arith::feq(ti.seconds(), 60))
  ti = 2 * ti;
  check(Arith::feq(ti.seconds(), 120))
  ti = TimeDuration::Seconds(60) + TimeDuration::Seconds(30);
  check(Arith::feq(ti.seconds(), 90))
  t = TimeDuration::Seconds(60) + Time::SecondsSinceEpoch(100);
  check(t.getSecondsSinceEpoch() == 160)
  ti += 30;
  check(Arith::feq(ti.seconds(), 120))
  ti -= 30;
  check(Arith::feq(ti.seconds(), 90))
  ti = TimeDuration::Seconds(60) - 30;
  check(Arith::feq(ti.seconds(), 30))

  /**
  ***  TimeDuration comparisons:
  ***  operator <
  ***  operator <=
  ***  operator ==
  ***  operator !=
  ***  operator >=
  ***  operator >
  **/

  ti = TimeDuration::Seconds(60);
  TimeDuration ti2 = TimeDuration::Seconds(61);
  check(ti < ti2)
  check(ti <= ti2)
  check(ti != ti2)
  check(!(ti == ti2))
  check(!(ti >= ti2))
  check(!(ti > ti2))

  ti = ti2;
  check(ti == ti2)
  check(ti <= ti2)
  check(ti >= ti2)
  check(!(ti < ti2))
  check(!(ti > ti2))
  check(!(ti != ti2))

  ti  = TimeDuration::Seconds(61);
  ti2 = TimeDuration::Seconds(60);
  check(ti > ti2)
  check(ti >= ti2)
  check(ti != ti2)
  check(!(ti == ti2))
  check(!(ti <= ti2))
  check(!(ti < ti2))

  const char * instr = "[2001 10/28 16:45:59 UTC]";
  const Time cd(instr);
  check(cd.getFileNameString() == "20011028-164559")
  check(cd.getFileNameString() == "20011028-164559")
  check(cd.getLogString() == "[2001 10/28 16:45:59 UTC]")

  const Time da(Time::fromStringForFileName("20011028-164559") );
  check(da.getFileNameString() == "20011028-164559")
  check(da.getFileNameString() == "20011028-164559")
  check(da.getLogString() == "[2001 10/28 16:45:59 UTC]")

  check(da == cd)
  check(!(da < cd))
  check(!(cd < da))
  check(cd.getYear() == 2001)
  check(cd.getMonth() == 10)
  check(cd.getDay() == 28)
  check(cd.getHour() == 16)
  check(cd.getMinute() == 45)
  check(cd.getSecond() == 59)

  std::string instr2 = "aaaaaa89030348sjf;jdf;jsfio3940384093843408 kjsf;ldfj;df39083049348";
  int offset = instr2.size();
  instr2 += instr;
  // const Date cdo (instr2, offset);
  const Time cdo(instr2, offset);
  check(da == cdo)

  // Current time test
  Time tnow = Time::CurrentTime();
  std::string logcurrent1 = tnow.getLogString();
  std::string filename1   = tnow.getFileNameString();
  long epoch1 = tnow.getSecondsSinceEpoch();

  Time t5 = Time(tnow.getYear(), tnow.getMonth(), tnow.getDay(), tnow.getHour(), tnow.getMinute(), tnow.getSecond());
  std::string logcurrent2 = t5.getLogString();
  std::string filename2   = t5.getFileNameString();
  long epoch2 = t5.getSecondsSinceEpoch();
  check(logcurrent1 == logcurrent2);
  check(filename1 == filename2);
  check(epoch1 == epoch2);

  Time t4(1562292000, 0); // 7/5/2019 2:00:00 am GMT/UTC
  check(t4.getYear() == 2019)
  check(t4.getMonth() == 7)
  check(t4.getDay() == 5)
  check(t4.getHour() == 2)
  check(t4.getMinute() == 0)
  check(t4.getSecond() == 0)
  check(t4.getSecondsSinceEpoch() == 1562292000);
} // TestTime::logicTest
}
