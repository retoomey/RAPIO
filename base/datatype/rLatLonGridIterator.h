#pragma once
#include <rLatLonGrid.h>
#include <rConstants.h>

namespace rapio {
class LatLonGridIterator;

class LatLonGridCallback {
public:
  virtual void
  handlePixel(LatLonGridIterator * it) = 0;

  // Default empty implementations so callers only override what they need
  virtual void handleBeginLoop(LatLonGridIterator * it, const LatLonGrid& grid){ }

  virtual void handleEndLoop(LatLonGridIterator * it, const LatLonGrid& grid){ }
};

class LatLonGridIterator {
public:
  LatLonGridIterator(LatLonGrid& grid);

  void
  setOutputArray(const std::string& key = Constants::PrimaryDataName);

  // Standard 2D traversal (Y -> X)
  void
  iterate(LatLonGridCallback& callback);

  inline void
  setValue(float v)
  {
    // Note: DataGrid 2D arrays are mapped [Y][X] (Lat/Lon) in memory
    (*myOutputArray)[myCurrentLatIdx][myCurrentLonIdx] = v;
  }

  // --- Index Access ---
  inline size_t
  getCurrentLatIdx() const { return myCurrentLatIdx; }

  inline size_t
  getCurrentLonIdx() const { return myCurrentLonIdx; }

  // --- Geolocation Access ---
  inline float
  getCurrentLatDegs() const { return myCurrentLatDegs; }

  inline float
  getCurrentLonDegs() const { return myCurrentLonDegs; }

  // --- Grid Metadata ---
  inline float
  getLatSpacing() const { return myLatSpacing; }

  inline float
  getLonSpacing() const { return myLonSpacing; }

  inline float
  getValue() const
  {
    return (*myOutputArray)[myCurrentLatIdx][myCurrentLonIdx];
  }

  // Relative access: 0,0 is current pixel.
  // +1 is North/East, -1 is South/West.
  inline float
  getRelativeValue(int dy, int dx) const
  {
    int ny = static_cast<int>(myCurrentLatIdx) + dy;
    int nx = static_cast<int>(myCurrentLonIdx) + dx;

    // The iterator hides all calculations and bounds checking
    if ((ny < 0) || (ny >= static_cast<int>(myGrid.getNumLats())) ||
      (nx < 0) || (nx >= static_cast<int>(myGrid.getNumLons())))
    {
      return rapio::Constants::DataUnavailable;
    }

    return (*myOutputArray)[ny][nx];
  }

private:
  LatLonGrid& myGrid;
  ArrayFloat2DPtr myOutputArray;

  size_t myCurrentLatIdx;
  size_t myCurrentLonIdx;

  float myCurrentLatDegs;
  float myCurrentLonDegs;

  float myLatSpacing;
  float myLonSpacing;
};
} // namespace rapio
