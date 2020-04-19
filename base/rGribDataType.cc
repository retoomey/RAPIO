#include "rGribDataType.h"
#include "rTime.h"
#include "rLLH.h"

using namespace rapio;

LLH
GribDataType::getLocation() const
{
  return LLH();
}

Time
GribDataType::getTime() const
{
  // FIXME: should be from grib data right?
  return Time();
}
