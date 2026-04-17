#pragma once
#include <rVolumeAlgorithm.h>
#include <rLatLonHeightGrid.h>
#include <rLatLonGrid.h> // terrain
#include <vector>
#include <string>
#include <memory>

namespace rapio {
/**
 * @class MaxAGL
 * @brief Calculates the maximum value in a column from the ground to a specified Above Ground Level (AGL) height.
 * * This algorithm takes a 3D LatLonHeightGrid (cube) and a 2D terrain grid (DEM).
 * It iterates up the columns of the 3D grid and records the maximum value found
 * between the terrain surface and the user-defined AGL ceiling.
 *
 * @author Robert Toomey
 */
class MaxAGL : public VolumeAlgorithm {
public:

  /**
   * @brief Default constructor. Initializes the algorithm with the name "MaxAGL".
   */
  MaxAGL() : VolumeAlgorithm("MaxAGL"){ }

  /**
   * @brief Declares command-line options specific to the MaxAGL algorithm.
   * @param o The RAPIOOptions object to populate.
   */
  virtual void
  declareOptions(RAPIOOptions& o) override;

  /**
   * @brief Processes the parsed command-line options.
   * @param o The populated RAPIOOptions object.
   */
  virtual void
  processOptions(RAPIOOptions& o) override;

  /** Get our iteration mode */
  virtual IterateMode
  getIterateMode() const override { return IterateMode::ColumnsDown; }

  /** Create our callback for doing work on the grid */
  virtual std::unique_ptr<LatLonHeightGridCallback>
  createCallback() override;

  /** Write our final products */
  virtual void
  writeFinalProducts(RAPIOAlgorithm * writer) override;

  /**
   * @brief Initializes or updates the output 2D grid based on the input cube's coverage.
   * @param input The input 3D data cube used as a spatial reference.
   */
  virtual void
  checkOutputGrids(std::shared_ptr<LatLonHeightGrid> input) override;

protected:

  float myTopAglKMs    = 4.0f;                     ///< The ceiling height AGL in kilometers to search up to.
  bool myWarnedTerrain = false;                    ///< Ensures we warn once
  std::shared_ptr<LatLonGrid> myMaxGrid = nullptr; ///< The output 2D grid storing the calculated maximums.
};
} // namespace rapio
