#include "rWgribDataTypeImp.h"
#include "rIOWgrib.h"

// Callbacks
#include "rCatalogCallback.h"
#include "rGridCallback.h"

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
  std::shared_ptr<GridCallback> action = std::make_shared<GridCallback>(myURL);

  action->execute();
  LogSevere("------------------end get float 2d\n");
  return nullptr;
}

std::shared_ptr<Array<float, 3> >
WgribDataTypeImp::getFloat3D(const std::string& key, std::vector<std::string> zLevels)
{
  return nullptr;

  size_t nz = zLevels.size();

  if ((nz < 1)) {
    LogSevere("Need at least 1 z level for 3D");
    return nullptr;
  }

  return nullptr;
}
