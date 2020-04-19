#include "rNetcdfDataTypeImp.h"
#include "rIONetcdf.h"

using namespace rapio;

LLH
NetcdfDataTypeImp::getLocation() const
{
  return LLH();
}

Time
NetcdfDataTypeImp::getTime() const
{
  return Time();
}
