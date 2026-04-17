#include "rLatLonHeightGridIterator.h"

using namespace rapio;

LatLonHeightGridIterator::LatLonHeightGridIterator(LatLonHeightGrid& grid, size_t startY, size_t endY)
  : myGrid(grid),
  myOutputArray(grid.getFloat3DPtr()),
  myStartY(startY),
  myEndY(endY)
{
  myLatSpacing = grid.getLatSpacing();
  myLonSpacing = grid.getLonSpacing();

  // If endY is 0, default to the full grid height
  if ((myEndY == 0) || (myEndY > grid.getNumLats())) {
    myEndY = grid.getNumLats();
  }

  // Create a cache (saves on /1000 in looping if working in KM)
  const size_t numLayers = grid.getNumLayers();

  myCachedHeightsKM.resize(numLayers);
  for (size_t z = 0; z < numLayers; ++z) {
    myCachedHeightsKM[z] = grid.getLayerValue(z) / 1000.0f;
  }
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
    myCurrentLayerIdx = z;
    // myCurrentHeightKMs = myGrid.getLayerValue(z) / 1000.0f;
    myCurrentHeightKMs = myCachedHeightsKM[z]; // use cache

    myCurrentLatDegs = startLat;
    // for (size_t y = 0; y < numLats; ++y, myCurrentLatDegs -= myLatSpacing) {
    for (size_t y = myStartY; y < myEndY; ++y, myCurrentLatDegs -= myLatSpacing) {
      myCurrentLatIdx = y;

      myCurrentLonDegs = startLon;
      for (size_t x = 0; x < numLons; ++x, myCurrentLonDegs += myLonSpacing) {
        myCurrentLonIdx = x;

        // Project/calculate current terrain for this voxel
        // With the mapping might actually be slower than columns, but
        // if we end up pre mapping it will become quick.
        if (myTerrainProj) {
          double terrM = myTerrainProj->getValueAtLL(myCurrentLatDegs, myCurrentLonDegs);
          myCurrentTerrainHeightKMs = Constants::isGood(terrM) ? (terrM / 1000.0f) : 0.0f;
        } else {
          myCurrentTerrainHeightKMs = 0.0f;
        }

        callback.handleVoxel(this);
      }
    }
  }

  callback.handleEndLoop(this, myGrid);
} // LatLonHeightGridIterator::iterateVoxels

template <bool IterateUp>
void
LatLonHeightGridIterator::iterateColumnsImpl(LatLonHeightGridCallback& callback)
{
  const size_t numLats   = myGrid.getNumLats();
  const size_t numLons   = myGrid.getNumLons();
  const size_t numLayers = myGrid.getNumLayers();

  LLH nw         = myGrid.getTopLeftLocationAt(0, 0);
  float startLat = nw.getLatitudeDeg() - (myLatSpacing / 2.0f);
  float startLon = nw.getLongitudeDeg() + (myLonSpacing / 2.0f);

  callback.handleBeginLoop(this, myGrid);

  myCurrentLatDegs = startLat;
  // for (size_t y = 0; y < numLats; ++y, myCurrentLatDegs -= myLatSpacing) {
  for (size_t y = myStartY; y < myEndY; ++y, myCurrentLatDegs -= myLatSpacing) {
    myCurrentLatIdx  = y;
    myCurrentLonDegs = startLon;
    for (size_t x = 0; x < numLons; ++x, myCurrentLonDegs += myLonSpacing) {
      myCurrentLonIdx = x;

      // Project/calculate current terrain for this column
      if (myTerrainProj) {
        double terrM = myTerrainProj->getValueAtLL(myCurrentLatDegs, myCurrentLonDegs);
        myCurrentTerrainHeightKMs = Constants::isGood(terrM) ? (terrM / 1000.0f) : 0.0f;
      } else {
        myCurrentTerrainHeightKMs = 0.0f;
      }

      callback.handleBeginColumn(this);

      // The compiler resolves this 'if' at compile-time and strips the dead branch!
      if (IterateUp) {
        for (size_t z = 0; z < numLayers; ++z) {
          myCurrentLayerIdx  = z;
          myCurrentHeightKMs = myGrid.getLayerValue(z) / 1000.0f;
          callback.handleVoxel(this);
        }
      } else {
        for (size_t z = numLayers; z-- > 0;) {
          myCurrentLayerIdx  = z;
          myCurrentHeightKMs = myGrid.getLayerValue(z) / 1000.0f;
          callback.handleVoxel(this);
        }
      }

      callback.handleEndColumn(this);
    }
  }
  callback.handleEndLoop(this, myGrid);
} // LatLonHeightGridIterator::iterateColumnsImpl

void
LatLonHeightGridIterator::iterateUpColumns(LatLonHeightGridCallback& callback)
{
  iterateColumnsImpl<true>(callback);
}

void
LatLonHeightGridIterator::iterateDownColumns(LatLonHeightGridCallback& callback)
{
  iterateColumnsImpl<false>(callback);
}
