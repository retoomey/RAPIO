// Add this at top for any BOOST test
#include <boost/test/unit_test.hpp>

#include "rTime.h"
#include "rArith.h"

using namespace rapio;

BOOST_AUTO_TEST_SUITE(_Time_)

BOOST_AUTO_TEST_CASE(_Time_Get_Set)
{
  Time t = rapio::Time::SecondsSinceEpoch(100);

  BOOST_CHECK_EQUAL(t.getSecondsSinceEpoch(), 100);
  BOOST_CHECK(Arith::feq(t.getFractional(), 0.0));
  t += TimeDuration::Minutes(2);
  BOOST_CHECK(t.getSecondsSinceEpoch() == 220);
}

BOOST_AUTO_TEST_CASE(_Time_Comparison_Operators_)
{
  Time t1(Time::SecondsSinceEpoch(100));
  Time t2(Time::SecondsSinceEpoch(101));

  t1 += TimeDuration::Seconds(0.5);
  t2 -= TimeDuration::Seconds(0.5);
  BOOST_CHECK(t1 == t2);
  BOOST_CHECK(t1 <= t2);
  BOOST_CHECK(t1 >= t2);
  BOOST_CHECK(!(t1 != t2));
  BOOST_CHECK(!(t1 < t2));
  BOOST_CHECK(!(t1 > t2));
}

BOOST_AUTO_TEST_CASE(_Time_SecMinHourDays_)
{
  Time t1(Time::SecondsSinceEpoch(100));
  Time t2(Time::SecondsSinceEpoch(101));

  t1 += TimeDuration::Seconds(0.5);
  t2 -= TimeDuration::Seconds(0.5);
  BOOST_CHECK(Arith::feq(TimeDuration::Seconds(129600).seconds(), 129600));
  BOOST_CHECK(Arith::feq(TimeDuration::Seconds(129600).minutes(), 2160));
  BOOST_CHECK(Arith::feq(TimeDuration::Seconds(129600).hours(), 36));
  BOOST_CHECK(Arith::feq(TimeDuration::Seconds(129600).days(), 1.5));
  BOOST_CHECK(Arith::feq(TimeDuration::Minutes(2.0).seconds(), 120));
  BOOST_CHECK(Arith::feq(TimeDuration::Hours(2.0).seconds(), 7200));
  BOOST_CHECK(Arith::feq(TimeDuration::Days(1.5).seconds(), 129600));
}

BOOST_AUTO_TEST_CASE(_Time_Change_Operators_)
{
  TimeDuration ti = TimeDuration::Seconds(60) / 2;

  BOOST_CHECK(Arith::feq(ti.seconds(), 30));
  ti /= 2;
  BOOST_CHECK(Arith::feq(ti.seconds(), 15));
  ti *= 2;
  BOOST_CHECK(Arith::feq(ti.seconds(), 30));
  ti = ti * 2;
  BOOST_CHECK(Arith::feq(ti.seconds(), 60));
  ti = 2 * ti;
  BOOST_CHECK(Arith::feq(ti.seconds(), 120));
  ti = TimeDuration::Seconds(60) + TimeDuration::Seconds(30);
  BOOST_CHECK(Arith::feq(ti.seconds(), 90));
  Time t = TimeDuration::Seconds(60) + Time::SecondsSinceEpoch(100);
  BOOST_CHECK(t.getSecondsSinceEpoch() == 160);
  ti += 30;
  BOOST_CHECK(Arith::feq(ti.seconds(), 120));
  ti -= 30;
  BOOST_CHECK(Arith::feq(ti.seconds(), 90));
  ti = TimeDuration::Seconds(60) - 30;
  BOOST_CHECK(Arith::feq(ti.seconds(), 30));
}

BOOST_AUTO_TEST_CASE(_TimeDuration_Comparisons_)
{
  TimeDuration ti  = TimeDuration::Seconds(60);
  TimeDuration ti2 = TimeDuration::Seconds(61);

  BOOST_CHECK(ti < ti2);
  BOOST_CHECK(ti <= ti2);
  BOOST_CHECK(ti != ti2);
  BOOST_CHECK(!(ti == ti2));
  BOOST_CHECK(!(ti >= ti2));
  BOOST_CHECK(!(ti > ti2));

  ti = ti2;
  BOOST_CHECK(ti == ti2);
  BOOST_CHECK(ti <= ti2);
  BOOST_CHECK(ti >= ti2);
  BOOST_CHECK(!(ti < ti2));
  BOOST_CHECK(!(ti > ti2));
  BOOST_CHECK(!(ti != ti2));

  ti  = TimeDuration::Seconds(61);
  ti2 = TimeDuration::Seconds(60);
  BOOST_CHECK(ti > ti2);
  BOOST_CHECK(ti >= ti2);
  BOOST_CHECK(ti != ti2);
  BOOST_CHECK(!(ti == ti2));
  BOOST_CHECK(!(ti <= ti2));
  BOOST_CHECK(!(ti < ti2));
}

BOOST_AUTO_TEST_CASE(_Time_LogStrings_)
{
  const char * instr = "[2001 10/28 16:45:59 UTC]";
  const Time cd(instr);

  BOOST_CHECK(cd.getFileNameString() == "20011028-164559");
  BOOST_CHECK(cd.getFileNameString() == "20011028-164559");
  BOOST_CHECK(cd.getLogString() == "[2001 10/28 16:45:59 UTC]");

  const Time da(Time::fromStringForFileName("20011028-164559") );
  BOOST_CHECK(da.getFileNameString() == "20011028-164559");
  BOOST_CHECK(da.getFileNameString() == "20011028-164559");
  BOOST_CHECK(da.getLogString() == "[2001 10/28 16:45:59 UTC]");

  BOOST_CHECK(da == cd);
  BOOST_CHECK(!(da < cd));
  BOOST_CHECK(!(cd < da));
  BOOST_CHECK(cd.getYear() == 2001);
  BOOST_CHECK(cd.getMonth() == 10);
  BOOST_CHECK(cd.getDay() == 28);
  BOOST_CHECK(cd.getHour() == 16);
  BOOST_CHECK(cd.getMinute() == 45);
  BOOST_CHECK(cd.getSecond() == 59);

  std::string instr2 = "aaaaaa89030348sjf;jdf;jsfio3940384093843408 kjsf;ldfj;df39083049348";
  int offset         = instr2.size();
  instr2 += instr;
  // const Date cdo (instr2, offset);
  const Time cdo(instr2, offset);
  BOOST_CHECK(da == cdo);
}

BOOST_AUTO_TEST_CASE(_Time_Current_)
{
  // Current time test
  Time tnow = Time::CurrentTime();

  std::string logcurrent1 = tnow.getLogString();
  std::string filename1   = tnow.getFileNameString();
  long epoch1 = tnow.getSecondsSinceEpoch();

  Time t5 = Time(tnow.getYear(), tnow.getMonth(), tnow.getDay(), tnow.getHour(), tnow.getMinute(), tnow.getSecond());
  std::string logcurrent2 = t5.getLogString();
  std::string filename2   = t5.getFileNameString();
  long epoch2 = t5.getSecondsSinceEpoch();
  BOOST_CHECK(logcurrent1 == logcurrent2);
  BOOST_CHECK(filename1 == filename2);
  BOOST_CHECK(epoch1 == epoch2);

  Time t4(1562292000, 0); // 7/5/2019 2:00:00 am GMT/UTC
  BOOST_CHECK(t4.getYear() == 2019);
  BOOST_CHECK(t4.getMonth() == 7);
  BOOST_CHECK(t4.getDay() == 5);
  BOOST_CHECK(t4.getHour() == 2);
  BOOST_CHECK(t4.getMinute() == 0);
  BOOST_CHECK(t4.getSecond() == 0);
  BOOST_CHECK(t4.getSecondsSinceEpoch() == 1562292000);
}

BOOST_AUTO_TEST_SUITE_END();
