#include "rGribDataTypeImp.h"
#include "rIOGrib.h"
#include "rGribAction.h"
#include "rGribDatabase.h"

#include <fstream>

using namespace rapio;

GribDataTypeImp::GribDataTypeImp(const URL& url, const std::vector<char>& buf, int mode) : myURL(url), myBuf(buf), myMode(mode), myHaveIDX(false)
{
    myDataType = "GribData";

    // Figure out the index name of the ".idx" file
    // We do the 'root' to get rid of .gz suffixes for example that we auto unwrap
    std::string root;
    std::string ext = OS::getRootFileExtension(myURL.toString(), root);
    myIndexURL = URL(root + "." + ext + ".idx");

    if (OS::isRegularFile(myIndexURL.toString())){
      LogInfo("Located IDX file at " << myIndexURL << ".\n");
      if (GribDatabase::readIDXFile(myIndexURL, myIDXMessages)){
        myHaveIDX = true;
      }else{
        LogInfo("Couldn't parse/read IDX file, using fallback database.\n");
      }
    }else{
      LogInfo("No IDX file found, using fallback database.\n");
    }

    // Hack to snag first time.
    // Gonna force message mode here for the time snag
    // Note this means direct URL reading breaks. Need field cleanup
    GribScanFirstMessage scan(this);

    IOGrib::scanGribDataFILE(myURL, &scan);
}

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
