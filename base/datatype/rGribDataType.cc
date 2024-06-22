#include "rGribDataType.h"

using namespace rapio;

Time
GribMessage::getTime() const
{
  Time theTime = Time(mySection1[5], // year
      mySection1[6],                 // month
      mySection1[7],                 // day
      mySection1[8],                 // hour
      mySection1[9],                 // minute
      mySection1[10],                // second
      0.0                            // fractional
  );

  return theTime;
}

std::string
GribMessage::getDateString(const std::string& pattern) const
{
  return (getTime().getString(pattern));
}

std::ostream&
rapio::operator << (std::ostream& os, GribMessage& p)
{
  return p.print(os);
}
