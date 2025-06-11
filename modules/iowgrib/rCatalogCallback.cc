#include "rCatalogCallback.h"
#include "rIOWgrib.h"
#include "rError.h"

#include <sstream>
#include <vector>

using namespace rapio;

void
CatalogCallback::execute()
{
  RAPIOCallbackCPP catalog(this);

  // convert pointer to integer and stream to string
  // string now holds a decimal address
  std::ostringstream oss;

  oss << reinterpret_cast<uintptr_t>(&catalog);
  std::string pointer = oss.str();

  // Catalog doesn't 'really' need a callback, but seems cleaner this way
  const char * argv[] = {
    "wgrib2",
    myFilename.c_str(), // in scope
    "-rapio",           // Use our callback patch
    pointer.c_str()     // Send address of our RapioCallback class.
  };
  int argc = sizeof(argv) / sizeof(argv[0]);

  std::ostringstream param;

  for (size_t i = 0; i < argc; ++i) {
    param << argv[i];
    if (i < argc - 1) { param << " "; }
  }
  LogInfo("Calling: " << param.str() << "\n");

  std::vector<std::string> result = IOWgrib::capture_vstdout_of_wgrib2(argc, argv);

  for (size_t i = 0; i < result.size(); ++i) {
    LogInfo("[wgrib2] " << result[i] << "\n");
  }
} // CatalogCallback::execute

void
CatalogCallback::handleInitialize(int * decode, int * latlon)
{
  std::cout << "[C++] Initialize grid callback\n";

  // For dumping catalog, we don't want decoding/unpacking of data
  *decode = 0;
  // For dumping catalog, we don't want the lat lon values
  *latlon = 0;
}

void
CatalogCallback::handleFinalize()
{
  std::cout << "[C++] Finalize catalog\n";
}

void
CatalogCallback::handleSetLatLon(double * lat, double * lon, size_t nx, size_t ny)
{ }

void
CatalogCallback::handleGetLLCoverageArea(double * nwLat, double * nwLon,
  double * seLat, double * seLon, double * dLat, double * dLon,
  int * nLat, int * nLon)
{ }

void
CatalogCallback::handleData(const float * data, int n)
{ }
