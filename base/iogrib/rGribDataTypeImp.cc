#include "rGribDataTypeImp.h"
#include "rIOGrib.h"

using namespace rapio;

void
GribDataTypeImp::printCatalog()
{
  GribCatalog test;

  IOGrib::scanGribData(myBuf, &test);
}

std::shared_ptr<Array<float, 2> >
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

std::shared_ptr<Array<float, 3> >
GribDataTypeImp::getFloat3D(const std::string& key, size_t& x, size_t& y, size_t& z,
  std::vector<std::string> zLevelsVec, float missing)
{
  return IOGrib::get3DData(myBuf, key, zLevelsVec, x, y, z, missing);
  // return nullptr;
}
