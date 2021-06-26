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
  ti += TimeDuration::Seconds(30);
  BOOST_CHECK(Arith::feq(ti.seconds(), 120));
  ti -= TimeDuration::Seconds(30);
  BOOST_CHECK(Arith::feq(ti.seconds(), 90));
  ti = TimeDuration::Seconds(60) - TimeDuration::Seconds(30);
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
  const char * format1 = "[%Y %m/%d %H:%M:%S UTC]";
  const char * format2 = "%Y%m%d-%H%M%S";

  const char * times1 = "[2001 10/28 16:45:59 UTC]";
  const char * times2 = "20011028-164559";

  const Time cd(times1, format1);
  const Time da(times2, format2);

  BOOST_CHECK(cd.getString(format2) == times2);

  BOOST_CHECK(da.getString(format1) == times1);

  // std::string test1 = "%Y%m%d-%H%M%S.%/ms";
  BOOST_CHECK(da == cd);
  BOOST_CHECK(!(da < cd));
  BOOST_CHECK(!(cd < da));
  BOOST_CHECK(cd.getYear() == 2001);
  BOOST_CHECK(cd.getMonth() == 10);
  BOOST_CHECK(cd.getDay() == 28);
  BOOST_CHECK(cd.getHour() == 16);
  BOOST_CHECK(cd.getMinute() == 45);
  BOOST_CHECK(cd.getSecond() == 59);
}

BOOST_AUTO_TEST_CASE(_Time_Current_)
{
  // Current time test
  Time tnow = Time::CurrentTime();

  std::string test1       = "[%Y %m/%d %H:%M:%S UTC]";
  std::string rec1        = "%Y%m%d-%H%M%S.%/ms";
  std::string logcurrent1 = tnow.getString(test1);
  std::string filename1   = tnow.getString(rec1);
  long epoch1 = tnow.getSecondsSinceEpoch();

  Time t5 = Time(tnow.getYear(), tnow.getMonth(), tnow.getDay(), tnow.getHour(), tnow.getMinute(), tnow.getSecond());
  std::string logcurrent2 = t5.getString(test1);
  std::string filename2   = t5.getString(rec1);
  long epoch2 = t5.getSecondsSinceEpoch();
  BOOST_CHECK(tnow.getYear() == t5.getYear());
  BOOST_CHECK(tnow.getMonth() == t5.getMonth());
  BOOST_CHECK(tnow.getDay() == t5.getDay());
  BOOST_CHECK(tnow.getMinute() == t5.getMinute());
  BOOST_CHECK(tnow.getSecond() == t5.getSecond());
  // BOOST_TEST_MESSAGE(" Compare " << logcurrent1 << " and " << logcurrent2);
  BOOST_CHECK(logcurrent1 == logcurrent2);
  tnow.putString(logcurrent1, test1); // put back

  // BOOST_TEST_MESSAGE(" Compare " << filename1 << " and " << filename2);
  // BOOST_CHECK(filename1 == filename2); ms diff
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
