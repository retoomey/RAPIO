#pragma once

#include <rVolumeAlgorithm.h>
#include <rLatLonHeightGrid.h>
#include <rLatLonGrid.h>
#include <rDataTypeGroup.h>
#include <rDataProjection.h>
#include <rPTreeData.h>
#include <memory>
#include <string>
#include <vector>

namespace rapio {
/**
 * @class LayerAverageAlg
 * @brief Port of legacy W2Merger LayerAverage / Isotherms algorithm to RAPIO.
 *
 * Reads <layeraverage> and <product> definitions from w2merger.xml and computes
 * 2D isotherm/layer products (at, echotop, cloudtop, depth, avg, lowestalt, max, min)
 * using 3D LatLonHeightGrid volumes and 2D NSE height fields managed by VolumeAlgorithm::myNSEGroup.
 */
class LayerAverageAlg : public VolumeAlgorithm {
public:
  struct HeightSpec {
    std::string productName;
    float       defaultMeters = 0.0f;
  };

  enum class OpType {
    At,
    EchoTop,
    CloudTop,
    Depth,
    Avg,
    LowestAlt,
    Max,
    Min
  };

  struct OperationConfig {
    OpType                      type = OpType::Avg;
    std::string                 productName;
    std::string                 heightProductName;
    float                       threshold    = 0.0f;
    int                         heightIndex1 = -1;
    int                         heightIndex2 = -1;

    std::shared_ptr<LatLonGrid> resultGrid;
    std::shared_ptr<LatLonGrid> heightGrid;
  };

  struct LayerGroupConfig {
    std::string                  nameOf3DGrid;
    std::string                  unit;
    std::string                  colormap;
    std::vector<OperationConfig> operations;
  };

  LayerAverageAlg();
  virtual
  ~LayerAverageAlg() = default;

  virtual void
  declareOptions(RAPIOOptions& o) override;
  virtual void
  processOptions(RAPIOOptions& o) override;

  virtual IterateMode
  getIterateMode() const override { return IterateMode::ColumnsUp; }

  virtual void
  checkOutputGrids(std::shared_ptr<LatLonHeightGrid> input) override;
  virtual std::unique_ptr<LatLonHeightGridCallback>
  createCallback() override;
  virtual void
  writeFinalProducts(RAPIOAlgorithm * writer) override;

  /** Load layeraverage configuration from an XML file path or config-relative path. */
  bool
  loadConfigFromFile(const std::string& path);

  /** Load layeraverage configuration directly from an XML string (useful for testing). */
  bool
  loadConfigFromXMLString(const std::string& xmlContent);

  /** Load layeraverage configuration from a parsed PTreeData document. */
  bool
  loadConfigFromPTree(std::shared_ptr<PTreeData> ptreeData);

  /** Load layeraverage configuration directly from a PTreeNode (e.g. <w2algxml> or <w2merger>). */
  bool
  loadConfigFromNode(const PTreeNode& node);

  const std::vector<LayerGroupConfig>&
  getLayerGroups() const { return myLayerGroups; }

protected:
  size_t
  registerHeightSpec(const std::string& productName, float defaultMeters);
  bool
  parseOperationNode(const PTreeNode& node, OperationConfig& outOp);

  std::vector<HeightSpec> myHeightSpecs;
  std::vector<LayerGroupConfig> myLayerGroups;
  std::string myCurrentInputTypeName;
  std::string myConfigFile;
};

using LayerAverage = LayerAverageAlg;
} // namespace rapio
