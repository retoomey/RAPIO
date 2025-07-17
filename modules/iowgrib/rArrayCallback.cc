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

  size_t i = (nlats - 1) * nlons; // Start at highest latitude row

  for (size_t lat = 0; lat < nlats; ++lat) {
    for (size_t lon = 0; lon < nlons; ++lon, ++i) {
      float& value = data[i];
      output[lat][lon] = std::isnan(value) ? Constants::MissingData : value;
    }
    i -= 2 * nlons; // Move to previous row (reversing lat)
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

    const auto layer = myLayerNumber;

    size_t i = (nlats - 1) * nlons; // Start at highest latitude row
    for (size_t lat = 0; lat < nlats; ++lat) {
      for (size_t lon = 0; lon < nlons; ++lon, ++i) {
        float& value = data[i];
        output[lat][lon][layer] = std::isnan(value) ? Constants::MissingData : value;
      }
      i -= 2 * nlons; // Move to previous row (reversing lat)
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
