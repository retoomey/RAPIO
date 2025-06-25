#include "rWgribDataTypeImp.h"
#include "rIOWgrib.h"

#include "rWgribMessageImp.h"

// Callbacks
#include "rCatalogCallback.h"
#include "rGridCallback.h"
#include "rArrayCallback.h"

#include <fstream>

using namespace rapio;

WgribDataTypeImp::WgribDataTypeImp(const URL& url) : myURL(url)
{
  myDataType = "GribData";
}

void
WgribDataTypeImp::printCatalog()
{
  std::shared_ptr<CatalogCallback> action = std::make_shared<CatalogCallback>(myURL, "");

  action->execute();
  auto c = action->getMatchCount();

  LogInfo("Catalog matched: " << c << " items\n");
}

std::shared_ptr<GribMessage>
WgribDataTypeImp::getMessage(const std::string& key, const std::string& levelstr)
{
  // FIXME: getMessage API interesting here, since the 'match' happens on a
  // field, not a message.  So really message number might be a better api.
  // -match "^23.1" for example?

  // Construct the -match by appending currently.
  // FIXME: Change the API to be more wgrib2 regex friendly.  For now
  // keeping it the same so I don't have to mess with original iogrib, etc.
  std::string match;

  if (!levelstr.empty()) {
    match = ":" + key + ":" + levelstr + ":"; // Old way
  } else {
    match = key; // New way
  }

  // Run a catalog and make sure we have a single match.  Any more and we warn and fail.
  // For example, we don't want a bad match field to decode the entire file for example,
  std::shared_ptr<CatalogCallback> catalog = std::make_shared<CatalogCallback>(myURL, match);

  catalog->execute();
  auto c = catalog->getMatchCount();

  if (c > 1) {
    LogSevere("Match of \"" << match << "\" matches " << c << " fields.  You need 1 match to read 2DArray\n");
    return nullptr;
  }
  // Running the catalog and matching one gives us the info section in catalog
  // Note, so far no decoding/etc. has taken place. We do that later
  auto m = std::make_shared<WgribMessageImp>(myURL, catalog->myMessageNumber, catalog->myFilePosition,
      catalog->mySection0, catalog->mySection1);

  return m;
} // WgribDataTypeImp::getMessage

std::shared_ptr<Array<float, 2> >
WgribDataTypeImp::getFloat2D(const std::string& key, const std::string& levelstr)
{
  LogSevere("------------------start get float 2d\n");

  // Construct the -match by appending currently.
  // FIXME: Change the API to be more wgrib2 regex friendly.  For now
  // keeping it the same so I don't have to mess with original iogrib, etc.
  std::string match;

  if (!levelstr.empty()) {
    match = ":" + key + ":" + levelstr + ":"; // Old way
  } else {
    match = key; // New way
  }

  // Run a catalog and make sure we have a single match.  Any more and we warn and fail.
  // For example, we don't want a bad match field to decode the entire file for example,
  // which could decode too much stuff and hang/freeze our realtime alg.
  std::shared_ptr<CatalogCallback> catalog = std::make_shared<CatalogCallback>(myURL, match);

  catalog->execute();
  auto c = catalog->getMatchCount();

  if (c > 1) {
    LogSevere("Match of \"" << match << "\" matches " << c << " fields.  You need 1 match to read 2DArray\n");
    return nullptr;
  }

  // Run array now on single match
  std::shared_ptr<ArrayCallback> action = std::make_shared<ArrayCallback>(myURL, match);

  action->execute();

  // Send back the array, clearing our ownership of it
  auto temp = ArrayCallback::myTemp2DArray;

  ArrayCallback::myTemp2DArray = nullptr;
  LogSevere("------------------end get float 2d\n");
  return temp;
} // WgribDataTypeImp::getFloat2D

std::shared_ptr<Array<float, 3> >
WgribDataTypeImp::getFloat3D(const std::string& key, std::vector<std::string> zLevels)
{
  LogSevere("------------------start get float 3d\n");

  size_t nz = zLevels.size();

  LogSevere("---3D is asking for " << nz << " levels\n");
  if ((nz < 1)) {
    LogSevere("Need at least 1 z level for 3D");
  }

  LogSevere("------------------end get float 2d\n");
  return nullptr;
}
