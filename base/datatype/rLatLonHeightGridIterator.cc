#include "rLatLonHeightGridIterator.h"

using namespace rapio;

LatLonHeightGridIterator::LatLonHeightGridIterator(LatLonHeightGrid& grid)
  : myGrid(grid),
  myOutputArray(grid.getFloat3DPtr())
{
  myLatSpacing = grid.getLatSpacing();
  myLonSpacing = grid.getLonSpacing();
}

void
LatLonHeightGridIterator::setOutputArray(const std::string& key)
{
  myOutputArray = myGrid.getFloat3DPtr(key);
}

void
LatLonHeightGridIterator::iterateVoxels(LatLonHeightGridCallback& callback)
{
  const size_t numLats   = myGrid.getNumLats();
  const size_t numLons   = myGrid.getNumLons();
  const size_t numLayers = myGrid.getNumLayers();

  LLH nw         = myGrid.getTopLeftLocationAt(0, 0);
  float startLat = nw.getLatitudeDeg() - (myLatSpacing / 2.0f);
  float startLon = nw.getLongitudeDeg() + (myLonSpacing / 2.0f);

  callback.handleBeginLoop(this, myGrid);

  // Memory-optimal loop: Z -> Y -> X
  for (size_t z = 0; z < numLayers; ++z) {
    myCurrentLayerIdx  = z;
    myCurrentHeightKMs = myGrid.getLayerValue(z) / 1000.0f;

    myCurrentLatDegs = startLat;
    for (size_t y = 0; y < numLats; ++y, myCurrentLatDegs -= myLatSpacing) {
      myCurrentLatIdx = y;

      myCurrentLonDegs = startLon;
      for (size_t x = 0; x < numLons; ++x, myCurrentLonDegs += myLonSpacing) {
        myCurrentLonIdx = x;
        callback.handleVoxel(this);
      }
    }
  }

  callback.handleEndLoop(this, myGrid);
}

void
LatLonHeightGridIterator::iterateUpColumns(LatLonHeightGridCallback& callback)
{
  const size_t numLats   = myGrid.getNumLats();
  const size_t numLons   = myGrid.getNumLons();
  const size_t numLayers = myGrid.getNumLayers();

  // Pre-calculate starting center points
  LLH nw         = myGrid.getTopLeftLocationAt(0, 0);
  float startLat = nw.getLatitudeDeg() - (myLatSpacing / 2.0f);
  float startLon = nw.getLongitudeDeg() + (myLonSpacing / 2.0f);

  callback.handleBeginLoop(this, myGrid);

  // We iterate Lat (Y) -> Lon (X) -> Layer (Z).
  // While Z->Y->X is better for raw memory cache locality (since the
  // underlying boost::multi_array is [Z][Y][X]), meteorological algorithms
  // (like MaxAGL, VIL, EchoTop) almost always perform column-based math.
  // Iterating Z on the innermost loop allows callbacks to track column state
  // simply by checking if (getCurrentLayerIdx() == 0).

  myCurrentLatDegs = startLat;
  for (size_t y = 0; y < numLats; ++y, myCurrentLatDegs -= myLatSpacing) {
    myCurrentLatIdx = y;

    myCurrentLonDegs = startLon;
    for (size_t x = 0; x < numLons; ++x, myCurrentLonDegs += myLonSpacing) {
      myCurrentLonIdx = x;

      callback.handleBeginColumn(this);
      for (size_t z = 0; z < numLayers; ++z) {
        myCurrentLayerIdx  = z;
        myCurrentHeightKMs = myGrid.getLayerValue(z) / 1000.0f;

        callback.handleVoxel(this);
      }
      callback.handleEndColumn(this);
    }
  }

  callback.handleEndLoop(this, myGrid);
} // LatLonHeightGridIterator::iterateUpColumns
