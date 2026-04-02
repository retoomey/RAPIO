#pragma once
#include <rLatLonHeightGrid.h>
#include <rConstants.h>

namespace rapio {
class LatLonHeightGridIterator;

/**
 * @brief Callback for a LatLonHeightGrid iterator using a visitor pattern.
 *
 * Implement this class to handle processing of each voxel within a
 * LatLonHeightGrid by subclassing and defining the `handleVoxel` method.
 * @ingroup rapio_utility
 * @brief Callback for a LatLonHeightGridIterator
 */
class LatLonHeightGridCallback {
public:
  virtual void
  handleVoxel(LatLonHeightGridIterator * it) = 0;
  virtual void handleBeginColumn(LatLonHeightGridIterator * it){ };
  virtual void handleEndColumn(LatLonHeightGridIterator * it){ };
  virtual void handleBeginLoop(LatLonHeightGridIterator *, const LatLonHeightGrid& grid){ };
  virtual void handleEndLoop(LatLonHeightGridIterator *, const LatLonHeightGrid& grid){ };
};

/**.
 * @brief Iterator for LatLonHeightGrid data with optimized performance.
 *
 * This iterator uses a visitor pattern for efficient processing of
 * large data sets, minimizing the overhead of standard iterators.
 *
 * @ingroup rapio_utility
 * @brief An iterator class for processing a LatLonHeightGrid
 */
class LatLonHeightGridIterator {
public:
  LatLonHeightGridIterator(LatLonHeightGrid& grid);

  /** Set the array for output of the iterator */
  void
  setOutputArray(const std::string& key = Constants::PrimaryDataName);

  // Iteration methods

  /** Iterate over voxels most efficiently. Possibly any order,
   * depending upon the underlying storage. */
  void
  iterateVoxels(LatLonHeightGridCallback& callback);

  /** Iterate over voxels, column Z last.  For column algorithms.
   * This also calls the handleBeginColumn and handleEndColumn
   * methods. */
  void
  iterateUpColumns(LatLonHeightGridCallback& callback);

  /** Set the primary data value */
  inline void
  setValue(float v)
  {
    // Note: DataGrid 3D arrays are mapped [Z][Y][X] in memory
    (*myOutputArray)[myCurrentLayerIdx][myCurrentLatIdx][myCurrentLonIdx] = v;
  }

  // --- Index Access ---
  inline size_t
  getCurrentLatIdx() const { return myCurrentLatIdx; }

  inline size_t
  getCurrentLonIdx() const { return myCurrentLonIdx; }

  inline size_t
  getCurrentLayerIdx() const { return myCurrentLayerIdx; }

  // --- Geolocation Access ---
  inline float
  getCurrentLatDegs() const { return myCurrentLatDegs; }

  inline float
  getCurrentLonDegs() const { return myCurrentLonDegs; }

  inline float
  getCurrentHeightKMs() const { return myCurrentHeightKMs; }

  // --- Grid Metadata ---
  inline float
  getLatSpacing() const { return myLatSpacing; }

  inline float
  getLonSpacing() const { return myLonSpacing; }

  /** Get the data value */
  inline float
  getValue()
  {
    // Note: DataGrid 3D arrays are mapped [Z][Y][X] in memory
    return (*myOutputArray)[myCurrentLayerIdx][myCurrentLatIdx][myCurrentLonIdx];
  }

  // Relative access: 0,0,0 is current voxel.
  // +1 is North/East/Up, -1 is South/West/Down.
  // No wrapping.  I'm assuming we'll never cover entire earth
  // at least for now.

  /**
   *
   *
   * float center = it->getRelativeValue(0, 0, 0);
   * float above  = it->getRelativeValue(0, 0, 1);
   * float north  = it->getRelativeValue(0, 1, 0);
   */
  inline float
  getRelativeValue(int dx, int dy, int dz) const
  {
    int nx = static_cast<int>(myCurrentLonIdx) + dx;
    int ny = static_cast<int>(myCurrentLatIdx) + dy;
    int nz = static_cast<int>(myCurrentLayerIdx) + dz;

    // The iterator hides all calculations and bounds checking
    if ((nx < 0) || (nx >= static_cast<int>(myGrid.getNumLons())) ||
      (ny < 0) || (ny >= static_cast<int>(myGrid.getNumLats())) ||
      (nz < 0) || (nz >= static_cast<int>(myGrid.getNumLayers())))
    {
      return rapio::Constants::DataUnavailable;
    }

    return (*myOutputArray)[nz][ny][nx];
  }

private:
  LatLonHeightGrid& myGrid;
  ArrayFloat3DPtr myOutputArray;

  size_t myCurrentLatIdx;
  size_t myCurrentLonIdx;
  size_t myCurrentLayerIdx;

  float myCurrentLatDegs;
  float myCurrentLonDegs;
  float myCurrentHeightKMs;

  float myLatSpacing;
  float myLonSpacing;
};
} // namespace rapio
