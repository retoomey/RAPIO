#include "rCatalogCallback.h"
#include "rIOWgrib.h"
#include "rError.h"

#include <sstream>
#include <vector>

using namespace rapio;

CatalogCallback::CatalogCallback(const URL& u, const std::string& match) : WgribCallback(u, match), myMatchCount(0)
{ }

void
CatalogCallback::handleSetDataArray(float * data, int nlats, int nlons, unsigned int * index)
{
  // Data and index will be empty
  myMatchCount++;
}

void
CatalogCallback::handleInitialize(int * decode, int * latlon)
{
  std::cout << "[C++] Initialize catalog\n";

  // For dumping catalog, we don't want decoding/unpacking of data
  *decode = 0;
  // For dumping catalog, we don't want the lat lon values
  *latlon = 0;

  myMatchCount = 0;
}

void
CatalogCallback::handleFinalize()
{
  std::cout << "[C++] Finalize catalog\n";
}
