#include "rGribDataType.h"
#include "rIOGrib.h"

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

void
GribDataType::printCatalog()
{
  GribCatalog test;

  IOGrib::scanGribData(myBuf, &test);
}

std::shared_ptr<RAPIO_2DF>
GribDataType::getFloat2D(const std::string& key, const std::string& levelstr)
{
  // Humm has vs is...a better way?
  GribMatcher test(key, levelstr);

  IOGrib::scanGribData(myBuf, &test);
  size_t at, fieldNumber;
  if (test.getMatch(at, fieldNumber)) {
    return IOGrib::get2DData(myBuf, at, fieldNumber);
  }
  return nullptr;
}
