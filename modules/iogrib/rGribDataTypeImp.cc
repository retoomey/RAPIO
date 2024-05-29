#include "rGribDataTypeImp.h"
#include "rIOGrib.h"
#include "rGribAction.h"

#include <fstream>

using namespace rapio;

void
GribDataTypeImp::scanGribData(GribAction * a)
{
  // Call the correct helper function for us
  if (myMode == 1) { // FULL read, use the prefilled in buffer
    IOGrib::scanGribData(myBuf, a);
  } else { // default to by message
    IOGrib::scanGribDataFILE(myURL, a);
  }
}

void
GribDataTypeImp::printCatalog()
{
  GribCatalog test;

  scanGribData(&test);
} // GribDataTypeImp::printCatalog

std::shared_ptr<GribMessage>
GribDataTypeImp::getMessage(const std::string& key, const std::string& levelstr)
{
  GribMatcher match1(key, levelstr);

  scanGribData(&match1);

  // Subclass humm
  auto m = match1.getMatchedMessage();

  return m;
}

std::shared_ptr<Array<float, 2> >
GribDataTypeImp::getFloat2D(const std::string& key, const std::string& levelstr)
{
  GribMatcher match1(key, levelstr);

  scanGribData(&match1);

  auto m = match1.getMatchedMessage();

  if (m != nullptr) {
    return IOGrib::get2DData(m, match1.getMatchedFieldNumber());
  }
  return nullptr;
}

std::shared_ptr<Array<float, 3> >
GribDataTypeImp::getFloat3D(const std::string& key, std::vector<std::string> zLevels)
{
  size_t nz = zLevels.size();

  if ((nz < 1)) {
    LogSevere("Need at least 1 z level for 3D");
    return nullptr;
  }

  GribNMatcher matchN(key, zLevels);

  scanGribData(&matchN);

  if (matchN.checkAllLevels()) {
    return IOGrib::get3DData(matchN.getMatchedMessages(), matchN.getMatchedFieldNumbers(), zLevels);
  }
  return nullptr;
}
