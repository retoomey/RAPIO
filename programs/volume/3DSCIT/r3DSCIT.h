#pragma once

#include <rVolumeAlgorithm.h>
#include <rLatLonHeightGrid.h>
#include <rDataTable.h>
#include <memory>
#include <string>

namespace rapio {
/**
 * @class SCITAlg
 * @brief Storm Cell Identification and Tracking (SCIT) 2D/3D feature extraction algorithm.
 *
 * Ports the legacy W2Merger SCIT algorithm into the RAPIO VolumeAlgorithm framework.
 * Iterates across vertical levels and 7 reflectivity thresholds (30-60 dBZ) using
 * 4-connected component labeling, area filtering, multi-threshold cluster compression,
 * and reflectivity-weighted centroid calculation, outputting a DataTable ("Scit2D").
 */
class SCITAlg : public VolumeAlgorithm {
public:
  /** Default constructor. Initializes the algorithm with the name "3DSCIT". */
  SCITAlg() : VolumeAlgorithm("3DSCIT"){ }

  /** Declare algorithm options and output products. */
  virtual void
  declareOptions(RAPIOOptions& o) override;

  /** Process parsed algorithm options. */
  virtual void
  processOptions(RAPIOOptions& o) override;

  /** Use voxel iteration to collect the full 3D volume for slice-by-slice 2D/3D clustering. */
  virtual IterateMode
  getIterateMode() const override { return IterateMode::Voxels; }

  /** Create the per-volume callback that performs SCIT clustering. */
  virtual std::unique_ptr<LatLonHeightGridCallback>
  createCallback() override;

  /** Write final Scit2D DataTable product. */
  virtual void
  writeFinalProducts(RAPIOAlgorithm * writer) override;

  /** Initialize or reset the output DataTable when coverage changes or on new volume. */
  virtual void
  checkOutputGrids(std::shared_ptr<LatLonHeightGrid> input) override;

protected:
  float myMinAreaKm2 = 15.0f; ///< Minimum cluster area threshold in km^2.
  std::shared_ptr<DataTable> myScitTable = nullptr;
};

using SCIT = SCITAlg;
} // namespace rapio
