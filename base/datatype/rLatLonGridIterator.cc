#include "rLatLonGridIterator.h"

using namespace rapio;

LatLonGridIterator::LatLonGridIterator(LatLonGrid& grid)
  : myGrid(grid),
  myOutputArray(grid.getFloat2DPtr())
{
  myLatSpacing = grid.getLatSpacing();
  myLonSpacing = grid.getLonSpacing();
}

void
LatLonGridIterator::setOutputArray(const std::string& key)
{
  myOutputArray = myGrid.getFloat2DPtr(key);
}

void
LatLonGridIterator::iterate(LatLonGridCallback& callback)
{
  const size_t numLats = myGrid.getNumLats();
  const size_t numLons = myGrid.getNumLons();

  // Pre-calculate starting center points
  LLH nw         = myGrid.getTopLeftLocationAt(0, 0);
  float startLat = nw.getLatitudeDeg() - (myLatSpacing / 2.0f);
  float startLon = nw.getLongitudeDeg() + (myLonSpacing / 2.0f);

  callback.handleBeginLoop(this, myGrid);

  // Memory-optimal loop: Y -> X (Latitude -> Longitude)
  // This matches the [Y][X] storage of boost::multi_array initialized
  // via DataGrid::init for LatLonGrids.
  myCurrentLatDegs = startLat;
  for (size_t y = 0; y < numLats; ++y, myCurrentLatDegs -= myLatSpacing) {
    myCurrentLatIdx = y;

    myCurrentLonDegs = startLon;
    for (size_t x = 0; x < numLons; ++x, myCurrentLonDegs += myLonSpacing) {
      myCurrentLonIdx = x;

      callback.handlePixel(this);
    }
  }

  callback.handleEndLoop(this, myGrid);
}
