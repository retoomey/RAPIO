#pragma once
#include <RAPIO.h>
#include <vector>
#include <string>
#include <memory>

namespace rapio {
/**
 * @class MaxAGL
 * @brief Computes the maximum value in a vertical column from ground level to a specified AGL height.
 * * Takes a 3D LatLonHeightGrid (e.g., Reflectivity) and a 2D terrain LatLonGrid.
 * It determines the MSL ground height at each pixel, calculates the AGL ceiling,
 * and finds the maximum value within that vertical window.
 *
 * @author Robert Toomey
 */
class MaxAGL : public RAPIOAlgorithm {
public:
  MaxAGL(){ }

  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;
  virtual void
  processOptions(rapio::RAPIOOptions& o) override;
  virtual void
  processNewData(rapio::RAPIOData& d) override;

protected:
  void
  processVolume(std::shared_ptr<LatLonHeightGrid> input);
  void
  loadTerrainGrid();

  std::string myTerrainFile;
  float myTopAglKMs;

  std::shared_ptr<LatLonGrid> myTerrainGrid;
  std::shared_ptr<LatLonGridProjection> myTerrainProj;

  LLCoverageArea myCachedCoverage;
  std::shared_ptr<LatLonGrid> myMaxGrid;
};
} // namespace rapio
