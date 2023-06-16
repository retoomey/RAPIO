#include "rGribDataTypeImp.h"
#include "rIOGrib.h"
#include "rGribAction.h"

#include <fstream>

using namespace rapio;

void
GribDataTypeImp::printCatalog()
{
  GribCatalog test;

  IOGrib::scanGribData(myBuf, &test);

  #if 0
  bool tryFile = false;

  if (tryFile) {
    // Try with URL method instead of buffer....
    // We could have two separate subclasses
    LogSevere("My URL is " << myURL << "\n");
    // a local file can read by scanning the file, vs buffering everything in ram
    // FIXME: Check local file...
    // std::fstream::file(myURL.getPath(), std::ios::in | std::ios::binary);
    // if (!file){
    //  LogSevere("Unable to read file at " << myURL << "\n");
    // }
    // file.close();
    // Using c functions vs fstream here since g2c is a c library
    FILE * file = fopen(myURL.getPath().c_str(), "r+b");
    if (file == nullptr) {
      LogSevere("Unable to read file at " << myURL << "\n");
    } else {
      IOGrib::scanGribDataFILE(file, nullptr);
    }
    // IOGrib::scanGribData(file, &test);
    fclose(file);

    exit(1);
  } else {
    IOGrib::scanGribData(myBuf, &test);
  }
  #endif // if 0
} // GribDataTypeImp::printCatalog

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
