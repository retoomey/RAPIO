#pragma once

#include <rVolumeAlgorithm.h>
#include <rLatLonHeightGrid.h>
#include <rLatLonGrid.h>
#include <memory>
#include <string>

namespace rapio {
/**
 * @class ColumnStatsAlg
 * @brief Single-pass column reduction plugin consolidating generic vertical statistics
 *        from legacy w2merger into one configurable VolumeAlgorithm.
 *
 * All operations stream in a single IterateMode::ColumnsDown pass with O(1) per-column
 * state (no column buffering), making this plugin fast both standalone and when loop-fused
 * inside RAPIOFusionAlgs.
 *
 * Mapping to legacy w2merger algorithms (selectable via --ops):
 *   - "max"    -> w2merger/vol_VerticalMaximum  (<TypeName>Composite, or "MaxAGL" when --topaglkm > 0)
 *   - "min"    -> w2merger/vol_VerticalMinimum  (<TypeName>CompositeMin, or "MinAGL" when --topaglkm > 0)
 *   - "absmax" -> w2merger/vol_VerticalAbsMax   (<TypeName>CompositeAbsMax, or "AbsMaxAGL" when --topaglkm > 0)
 *   - "avg"    -> w2merger/vol_VerticalAverage  (<TypeName>Average, or "AvgAGL" when --topaglkm > 0)
 *   - "depth"  -> w2merger/vol_VerticalDepth    (<TypeName>Depth, or "DepthAGL" when --topaglkm > 0)
 *   - "lowht"  -> w2merger/vol_LowHtRefl        (LowHtRefl, or "LowHtAGL" when --topaglkm > 0)
 *   - "all"    -> shorthand enabling all of the above operations
 *
 * When --topaglkm > 0, all enabled operations restrict their column scan to
 * [terrain + bottomaglkm, terrain + topaglkm] (replacing the retired 3DMaxAGL,
 * MinAGL, and AbsMaxAGL plugins).
 */
class ColumnStatsAlg : public VolumeAlgorithm {
public:
  /** Construct the 3DColumnStats algorithm. */
  ColumnStatsAlg();
  virtual
  ~ColumnStatsAlg() = default;

  /** Declare algorithm options (--ops, --threshold, --topaglkm, plus VolumeAlgorithm options). */
  virtual void
  declareOptions(RAPIOOptions& o) override;

  /** Process parsed algorithm options. */
  virtual void
  processOptions(RAPIOOptions& o) override;

  /** Stream top-down so all stats (including lowht) and FusionAlgs share one scan direction. */
  virtual IterateMode
  getIterateMode() const override { return IterateMode::ColumnsDown; }

  /** Initialize or resize enabled 2D output grids when coverage or input product changes. */
  virtual void
  checkOutputGrids(std::shared_ptr<LatLonHeightGrid> input) override;

  /** Create the per-volume streaming callback for the enabled operations. */
  virtual std::unique_ptr<LatLonHeightGridCallback>
  createCallback() override;

  /** Write all enabled 2D output grids. */
  virtual void
  writeFinalProducts(RAPIOAlgorithm * writer) override;

  /** Parse and apply a comma-separated operations string (e.g. "max,min,avg" or "all"). */
  bool
  parseOpsString(const std::string& opsStr);

  /** Set the threshold used by "depth" (vol_VerticalDepth) and "lowht" (vol_LowHtRefl). */
  void setThreshold(float threshold){ myThreshold = threshold; }

  /** Set optional bottom height AGL in km (used when topaglkm > 0; default 0.0 km). */
  void setBottomAglKMs(float bottomAglKMs){ myBottomAglKMs = bottomAglKMs; }

  /** Set optional AGL ceiling in km (<= 0 disables AGL capping and scans the full column). */
  void setTopAglKMs(float topAglKMs){ myTopAglKMs = topAglKMs; }

  bool
  doMax() const { return myDoMax; }

  bool
  doMin() const { return myDoMin; }

  bool
  doAbsMax() const { return myDoAbsMax; }

  bool
  doAvg() const { return myDoAvg; }

  bool
  doDepth() const { return myDoDepth; }

  bool
  doLowHt() const { return myDoLowHt; }

protected:
  // Enabled operation flags (constant across a volume for predictable hot-loop branching)
  bool myDoMax    = true;  ///< w2merger/vol_VerticalMaximum (and optional AGL cap)
  bool myDoMin    = false; ///< w2merger/vol_VerticalMinimum
  bool myDoAbsMax = false; ///< w2merger/vol_VerticalAbsMax
  bool myDoAvg    = false; ///< w2merger/vol_VerticalAverage
  bool myDoDepth  = false; ///< w2merger/vol_VerticalDepth
  bool myDoLowHt  = false; ///< w2merger/vol_LowHtRefl

  float myThreshold    = 55.0f; ///< Reflectivity/value threshold for depth & lowht (legacy default: 55)
  float myBottomAglKMs = 0.0f;  ///< Bottom height AGL in km (used when myTopAglKMs > 0)
  float myTopAglKMs    = -1.0f; ///< Optional AGL ceiling in km (<= 0 means uncapped full column)
  bool myWarnedTerrain = false;

  std::string myCachedInputTypeName;

  // Cached 2D output grids (allocated only for enabled operations)
  std::shared_ptr<LatLonGrid> myMaxGrid;    ///< vol_VerticalMaximum output
  std::shared_ptr<LatLonGrid> myMinGrid;    ///< vol_VerticalMinimum output
  std::shared_ptr<LatLonGrid> myAbsMaxGrid; ///< vol_VerticalAbsMax output
  std::shared_ptr<LatLonGrid> myAvgGrid;    ///< vol_VerticalAverage output
  std::shared_ptr<LatLonGrid> myDepthGrid;  ///< vol_VerticalDepth output
  std::shared_ptr<LatLonGrid> myLowHtGrid;  ///< vol_LowHtRefl output
};

using ColumnStats = ColumnStatsAlg;
} // namespace rapio
