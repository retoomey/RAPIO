#include "rGribDataTypeImp.h"
#include "rIOGrib.h"

using namespace rapio;

LLH
GribDataTypeImp::getLocation() const
{
  return LLH();
}

Time
GribDataTypeImp::getTime() const
{
  // FIXME: should be from grib data right?
  return Time();
}

void
GribDataTypeImp::printCatalog()
{
  GribCatalog test;

  IOGrib::scanGribData(myBuf, &test);
}

std::shared_ptr<RAPIO_2DF>
GribDataTypeImp::getFloat2D(const std::string& key, const std::string& levelstr, size_t&x, size_t&y)
{
  // Humm has vs is...a better way?
  GribMatcher test(key, levelstr);

  IOGrib::scanGribData(myBuf, &test);
  size_t at, fieldNumber;
  if (test.getMatch(at, fieldNumber)) {
    return IOGrib::get2DData(myBuf, at, fieldNumber, x, y);
  }
  return nullptr;
}
