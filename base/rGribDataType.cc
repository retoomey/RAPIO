#include "rGribDataType.h"

using namespace rapio;

Time
GribMessage::getTime()
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
GribMessage::getDateString(const std::string& pattern)
{
  return (getTime().getString(pattern));
}
