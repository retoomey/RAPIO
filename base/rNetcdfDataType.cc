#include "rNetcdfDataType.h"
#include "rIONetcdf.h"

#include <vector>
#include <string>

using namespace rapio;

LLH
NetcdfDataType::getLocation() const
{
  return LLH();
}

Time
NetcdfDataType::getTime() const
{
  return Time();
}
