#include <rRAPIOAlgorithm.h>
#include <rLLCoverageArea.h>
#include <rLatLonGrid.h>
#include <rLatLonGridProjection.h>

#include <memory>
#include <string>
#include <map>
#include <mutex>

namespace rapio {
class LatLonHeightGrid;

/*
 * A VolumeAlgorithm is designed to iterate over a LatLonHeightGrid
 * It may be called standalone, or as part of a group of algorithms
 * say in fusion.
 *
 * Note: For the group of algorithms, you can assume shared coverage
 * area. For example, Fusion will send the same grid to N algorithms.
 *
 * We implement some common ability for the 3D grids.
 * 1.  Optional terrain file reading/processing
 * 2.  NSE value support (to do)
 *
 * @author Robert Toomey
 * @ingroup rapio_algorithm
 * @brief API implementation of a standard grid volume algorithm
 */

class VolumeAlgorithm : public RAPIOAlgorithm {
public:
  VolumeAlgorithm(const std::string& display = "Volume Algorithm")
    : RAPIOAlgorithm(display){ }

  virtual
  ~VolumeAlgorithm() = default;

  /** Declare common options */
  virtual void
  declareOptions(RAPIOOptions& o) override;

  /** Process common options */
  virtual void
  processOptions(RAPIOOptions& o) override;

  /** Preparing we do for incoming data */
  virtual void
  setupVolumeProcessing(std::shared_ptr<LatLonHeightGrid> llhg);

  /** All volume grid algorithms process volumes and typically
   * nothing else, so default to that */
  virtual void
  processNewData(RAPIOData& d) override;

  /** FusionAlgs will call this directly, passing itself as the writer.
   * This is the worker for the LatLonHeightGrid */
  virtual void
  processVolume(std::shared_ptr<LatLonHeightGrid> inputCube,
    RAPIOAlgorithm *                              writer) = 0;

protected:

  /**
   * @brief Hook for subclasses to initialize or resize their output grids.
   * Automatically called by processNewData before processVolume.
   */
  virtual void checkOutputGrids(std::shared_ptr<LatLonHeightGrid> input){ }

  /** @brief Utility method to check for grid changes between calls */
  bool
  checkCoverageChange(std::shared_ptr<LatLonHeightGrid> input);

  /**
   * @brief Loads the DEM grid into memory, utilizing a static cache to prevent
   * duplicate allocations across multiple algorithm instances.
   */
  void
  loadTerrainGrid();

  /** Remember the last coverage area for changes */
  LLCoverageArea myCachedCoverage;

  /** Name of the terrain file */
  std::string myTerrainFile;

  /** The current terrain grid */
  std::shared_ptr<LatLonGrid> myTerrainGrid;

  /** The current terrain projection (for values) */
  std::shared_ptr<LatLonGridProjection> myTerrainProj;

  // Pre-computed height arrays for 3D iterations
  // We should probably just have one of these
  std::vector<float> myHLevels;    ///< Height levels in meters
  std::vector<float> myHLevelsKMs; ///< Height levels in kilometers

private:
  // Note: This is because Fusion might call N VolumeAlgorithms, and several
  // may want terrain.  We don't want to duplicate terrain storage per instance.
  // So we have a satic cache to share terrain data across all VolumeAlgorithm instances
  static std::map<std::string, std::shared_ptr<LatLonGrid> > ourTerrainGridCache;
  static std::map<std::string, std::shared_ptr<LatLonGridProjection> > ourTerrainProjCache;
  static std::mutex ourTerrainMutex;
};
}
