#pragma once

#include <rVolumeAlgorithm.h>
#include <rLatLonHeightGrid.h>
#include <rLatLonGrid.h>
#include <rDataTypeGroup.h>
#include <memory>
#include <string>

namespace rapio {
/**
 * @class HDAAlg
 * @brief Calculates Hail Diagnosis products (SHI, POSH, MESH) from 3D Reflectivity cubes
 *        and 2D environmental height fields (0C, -20C, Terrain, and optional storm tilt).
 */
class HDAAlg : public VolumeAlgorithm {
public:
  /** Construct the 3DHDA algorithm. */
  HDAAlg();

  /** Declare all algorithm options. */
  virtual void
  declareOptions(RAPIOOptions& o) override;

  /** Process all algorithm options. */
  virtual void
  processOptions(RAPIOOptions& o) override;

  /** Iterate down columns (matches 3DVIL/MaxAGL pattern). */
  virtual IterateMode
  getIterateMode() const override { return IterateMode::ColumnsDown; }

  /** Create the per-volume callback that performs column integration. */
  virtual std::unique_ptr<LatLonHeightGridCallback>
  createCallback() override;

  /** Write final 2D hail products. */
  virtual void
  writeFinalProducts(RAPIOAlgorithm * writer) override;

  /** Initialize or resize output grids when coverage changes. */
  virtual void
  checkOutputGrids(std::shared_ptr<LatLonHeightGrid> input) override;

protected:
  std::shared_ptr<LatLonGrid> myShigrid;
  std::shared_ptr<LatLonGrid> myPoshgrid;
  std::shared_ptr<LatLonGrid> myMeshgrid;

  // Optional tilt-adjusted outputs
  std::shared_ptr<LatLonGrid> myShiTiltedGrid;
  std::shared_ptr<LatLonGrid> myPoshTiltedGrid;
  std::shared_ptr<LatLonGrid> myMeshTiltedGrid;

  bool myDoTilt = false;

  // Configurable NSE product names and fallback defaults (meters)
  std::string myH273Name    = "Heightof0C";
  std::string myH253Name    = "Heightof-20C";
  std::string myTerrainName = "HeightTerrain";
  std::string mySlopeWEName = "SlopeWE";
  std::string mySlopeNSName = "SlopeNS";

  float myDef273     = 4000.0f;
  float myDef253     = 7000.0f;
  float myDefTerrain = 0.0f;
};

using HDA = HDAAlg;
} // namespace rapio
