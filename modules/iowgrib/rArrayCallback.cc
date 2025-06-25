#include "rArrayCallback.h"
#include "rURL.h"
#include "rError.h"
#include "rIOWgrib.h"

#include <sstream>

using namespace rapio;

std::shared_ptr<Array<float, 2> > ArrayCallback::myTemp2DArray;

ArrayCallback::ArrayCallback(const URL& u, const std::string& match) : WgribCallback(u, match)
{ }

void
ArrayCallback::handleInitialize(int * decode, int * latlon)
{
  std::cout << "[C++] Initialize array\n";
  // For grids, we want decoding/unpacking of data
  *decode = 1;
  // For grids, we want the lat lon values
  // Actually do we for array?  We're not projecting data
  *latlon = 1;
}

void
ArrayCallback::handleFinalize()
{
  std::cout << "[C++] Finalize array\n";
}

void
ArrayCallback::handleSetDataArray(float * data, int nlats, int nlons, unsigned int * index)
{
  myTemp2DArray = Arrays::CreateFloat2D(nlats, nlons);
  auto& output = myTemp2DArray->ref();

  for (int lat = 0; lat < nlats; ++lat) {
    for (int lon = 0; lon < nlons; ++lon) {
      const size_t flipLat = nlats - (lat + 1);
      const int i  = flipLat * nlons + lon; // row-major
      float& value = data[i];

      if (std::isnan(value)) {
        output[lat][lon] = Constants::MissingData;
      } else {
        output[lat][lon] = value;
      }
    }
  }
}
