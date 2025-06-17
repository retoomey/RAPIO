#include "rWgribDataTypeImp.h"
#include "rIOWgrib.h"

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
  std::shared_ptr<CatalogCallback> action = std::make_shared<CatalogCallback>(myURL);

  action->execute();
}

std::shared_ptr<GribMessage>
WgribDataTypeImp::getMessage(const std::string& key, const std::string& levelstr)
{
  LogSevere("Message not implemented yet with Wgrib2\n");
  return nullptr;
}

std::shared_ptr<Array<float, 2> >
WgribDataTypeImp::getFloat2D(const std::string& key, const std::string& levelstr)
{
  LogSevere("------------------start get float 2d\n");

  // Construct the -match by appending currently.
  // FIXME: Change the API to be more wgrib2 regex friendly.  For now
  // keeping it the same so I don't have to mess with original iogrib, etc.
  std::string match = ":" + key + ":" + levelstr + ":";

  // FIXME: In real life we'll probably need to run catalog first and count the
  // matches.  If not what's expected, don't call the Grid because we don't want to
  // decode too much stuff and hang/freeze our realtime alg.
  // std::shared_ptr<GridCallback> action = std::make_shared<GridCallback>(myURL, match);
  std::shared_ptr<ArrayCallback> action = std::make_shared<ArrayCallback>(myURL, match);

  action->execute();

  // Send back the array, clearing our ownership of it
  auto temp = ArrayCallback::myTemp2DArray;

  ArrayCallback::myTemp2DArray = nullptr;
  LogSevere("------------------end get float 2d\n");
  return temp;
}

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
