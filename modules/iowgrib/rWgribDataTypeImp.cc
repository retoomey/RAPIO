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

std::shared_ptr<CatalogCallback>
WgribDataTypeImp::haveSingleMatch(const std::string& match)
{
  // Run a catalog and make sure we have a single match.  Any more and we warn and fail.
  // For example, we don't want a bad match field to decode the entire file for example,
  // which could decode too much stuff and hang/freeze our realtime alg.
  std::shared_ptr<CatalogCallback> catalog = std::make_shared<CatalogCallback>(myURL, match);

  catalog->execute();
  auto c = catalog->getMatchCount();

  if (c != 1) {
    LogSevere("\"" << match << "\" matches " << c << " field(s). You need an exact match\n");
    return nullptr;
  }
  return catalog;
}

std::shared_ptr<GribMessage>
WgribDataTypeImp::getMessage(const std::string& key, const std::string& levelstr)
{
  // Construct the -match by appending currently.
  // FIXME: Change the API to be more wgrib2 regex friendly.  For now
  // keeping it the same so I don't have to mess with original iogrib, etc.
  const std::string match = levelstr.empty() ? key : ":" + key + ":" + levelstr + ":";

  auto c = haveSingleMatch(match);

  if (!c) { return nullptr; }
  auto N = c->getMessageNumber();

  // Run catalog again with regex to match message number fields "^N:" or "^N.F:"
  // This gives us a count of the fields in the message
  std::string match2 = "^" + std::to_string(N) + "(\\.[0-9]+)?:";
  CatalogCallback c2(myURL, match2);

  c2.execute();

  // Running the catalog and matching one gives us the info section in catalog
  // Note, so far no decoding/etc. has taken place. We do that later
  auto m = std::make_shared<WgribMessageImp>(myURL, N, c2.getMatchCount(),
      c->getFilePosition(), c->getSection0(), c->getSection1());

  return m;
} // WgribDataTypeImp::getMessage

std::shared_ptr<Array<float, 2> >
WgribDataTypeImp::getFloat2D(const std::string& key, const std::string& levelstr)
{
  // FIXME: Change the API to be more wgrib2 regex friendly.  For now
  // keeping it the same so I don't have to mess with original iogrib, etc.
  const std::string match = levelstr.empty() ? key : ":" + key + ":" + levelstr + ":";

  auto c = haveSingleMatch(match);

  if (!c) { return nullptr; }

  // Run array now on single match
  std::shared_ptr<ArrayCallback> action = std::make_shared<ArrayCallback>(myURL, match);

  action->execute();

  // Send back the array, clearing our ownership of it
  auto temp = ArrayCallback::myTemp2DArray;

  ArrayCallback::myTemp2DArray = nullptr;
  return temp;
} // WgribDataTypeImp::getFloat2D

std::shared_ptr<Array<float, 3> >
WgribDataTypeImp::getFloat3D(const std::string& key, std::vector<std::string> zLevels)
{
  LogSevere("Float 3D direct not implemented yet with iowgrib module\n");

  return nullptr;

  #if 0
  size_t nz = zLevels.size();

  if ((nz < 1)) {
    LogSevere("Need at least 1 z level for 3D");
  }

  return nullptr;

  #endif
}

std::shared_ptr<LatLonGrid>
WgribDataTypeImp::getLatLonGrid(const std::string& key, const std::string& levelstr)
{
  // FIXME: Change the API to be more wgrib2 regex friendly.  For now
  // keeping it the same so I don't have to mess with original iogrib, etc.
  const std::string match = levelstr.empty() ? key : ":" + key + ":" + levelstr + ":";

  auto c = haveSingleMatch(match);

  if (!c) { return nullptr; }

  std::shared_ptr<GridCallback> action = std::make_shared<GridCallback>(myURL, match);

  action->execute();

  // Send back the LatLonGrid, clearing our ownership of it
  auto temp = GridCallback::myTempLatLonGrid;

  GridCallback::myTempLatLonGrid = nullptr;
  return temp;
}
