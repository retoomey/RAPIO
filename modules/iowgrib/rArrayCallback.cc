#include "rArrayCallback.h"
#include "rURL.h"
#include "rError.h"
#include "rIOWgrib.h"

#include <sstream>

using namespace rapio;

std::shared_ptr<Array<float, 2> > Array2DCallback::myTemp2DArray;
std::shared_ptr<Array<float, 3> > Array3DCallback::myTemp3DArray;

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
Array2DCallback::handleSetDataArray(float * data, int nlats, int nlons, unsigned int * index)
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

void
Array3DCallback::handleSetDataArray(float * data, int nlats, int nlons, unsigned int * index)
{
  if (myLayerNumber == 0) {
    // Found first match.  Create the array.
    myTemp3DArray = Arrays::CreateFloat3D(nlats, nlons, myLayers.size());
    myNLats       = nlats;
    myNLons       = nlons;
  }

  // Check that nlats/nlons matches what we expect
  if ((nlats != myNLats) || (nlons != myNLons)) {
    std::cerr << "\nError: (For layer " << myLayerNumber << " mismatched grid size " << nlats << "!= " << myNLats
              << " or " << nlons << " != " << myNLons << ")\n";
    return;
  }

  if (myTemp3DArray != nullptr) {
    auto& output = myTemp3DArray->ref();

    const auto layer         = myLayerNumber;
    const size_t layerOffset = layer * nlats * nlons;

    for (size_t lat = 0, flippedLat = nlats - 1; lat < nlats; ++lat, --flippedLat) {
      size_t rowOffset = layerOffset + flippedLat * nlons;
      for (size_t lon = 0; lon < nlons; ++lon) {
        size_t i     = rowOffset + lon;
        float& value = data[i];

        if (std::isnan(value)) {
          output[lat][lon][layer] = Constants::MissingData;
        } else {
          output[lat][lon][layer] = value;
        }
      }
    }
  }
} // Array3DCallback::handleSetDataArray

void
Array3DCallback::executeLayer(size_t layer)
{
  std::string holdMatch = myMatch; // Original key

  myMatch       = myMatch + ":" + myLayers[layer] + ":";
  myLayerNumber = layer;

  execute();
  myMatch = holdMatch;
}
