#include "r3DLayerAverage.h"
#include "rIODataType.h"
#include "rIOXML.h"
#include "rRAPIORuntime.h"
#include "rConfig.h"
#include "rLatLonHeightGridIterator.h"
#include "rDataTypeHistory.h"
#include "rConstants.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

using namespace rapio;

extern "C" {
RAPIOAlgorithm *
createRAPIOAlg(void)
{
  return reinterpret_cast<RAPIOAlgorithm *>(new rapio::LayerAverageAlg());
}
}

namespace {
/** Simple glob-style wildcard matcher supporting '*' and '?'. */
bool
wildcardMatches(const std::string& pattern, const std::string& text)
{
  if (pattern.empty()) {
    return true;
  }
  size_t p = 0, t = 0;
  size_t starIdx  = std::string::npos;
  size_t matchIdx = 0;

  while (t < text.size()) {
    if ((p < pattern.size()) && ((pattern[p] == '?') || (pattern[p] == text[t]))) {
      ++p;
      ++t;
    } else if ((p < pattern.size()) && (pattern[p] == '*') ) {
      starIdx  = p++;
      matchIdx = t;
    } else if (starIdx != std::string::npos) {
      p = starIdx + 1;
      t = ++matchIdx;
    } else {
      return false;
    }
  }
  while (p < pattern.size() && pattern[p] == '*') {
    ++p;
  }
  return p == pattern.size();
}

/** Linearly interpolate/extrapolate height offset between two dBZ levels. */
inline float
interpHeightOffset(
  float heightAt,
  float heightLast,
  float dBZAt,
  float dBZLast,
  float threshold)
{
  const float hdiff = heightLast - heightAt;
  const float p     = std::fabs(dBZAt - dBZLast);

  if (p > 0.01f) {
    return std::fabs((dBZAt - threshold) / p * hdiff);
  }
  return 0.0f;
}

/** Echo top search from top down (returns km, matching vol_Echoes.h::doEchoTop). */
float
computeEchoTop(
  const std::vector<float>& col,
  const std::vector<float>& hlevelsM,
  float                   threshold)
{
  const int numhts = static_cast<int>(col.size());

  if (numhts <= 0) {
    return Constants::DataUnavailable;
  }

  float lastH   = -10000.0f;
  float lastdBZ = 0.0f;
  float h       = Constants::DataUnavailable;

  for (int k = numhts - 1; k >= 0; --k) {
    const float dBZAt = col[k];
    if (dBZAt == Constants::DataUnavailable) {
      continue;
    }
    h = Constants::MissingData;

    if (dBZAt >= threshold) {
      if (lastH > -1000.0f) {
        h = hlevelsM[k] + interpHeightOffset(hlevelsM[k], lastH, dBZAt, lastdBZ, threshold);
        if (h > hlevelsM[numhts - 1]) {
          h = hlevelsM[numhts - 1];
        }
        h /= 1000.0f;
      } else {
        h = hlevelsM[k] / 1000.0f;
      }
      break;
    } else if (Constants::isGood(dBZAt)) {
      lastH   = hlevelsM[k];
      lastdBZ = dBZAt;
    }
  }
  return h;
} // computeEchoTop

/** Echo bottom search from bottom up (returns km, matching vol_Echoes.h::doEchoBottom). */
float
computeEchoBottom(
  const std::vector<float>& col,
  const std::vector<float>& hlevelsM,
  float                   threshold)
{
  const int numhts = static_cast<int>(col.size());

  if (numhts <= 0) {
    return Constants::DataUnavailable;
  }

  float lastH   = -10000.0f;
  float lastdBZ = 0.0f;
  float h       = Constants::DataUnavailable;

  for (int k = 0; k < numhts; ++k) {
    const float dBZAt = col[k];
    if (dBZAt == Constants::DataUnavailable) {
      continue;
    }
    h = Constants::MissingData;

    if (dBZAt >= threshold) {
      if (lastH > -1000.0f) {
        h  = hlevelsM[k] - interpHeightOffset(hlevelsM[k], lastH, dBZAt, lastdBZ, threshold);
        h /= 1000.0f;
      } else {
        h = hlevelsM[k] / 1000.0f;
      }
      break;
    } else if (Constants::isGood(dBZAt)) {
      lastH   = hlevelsM[k];
      lastdBZ = dBZAt;
    }
  }
  return h;
} // computeEchoBottom

/** Highest altitude with any valid data (returns km). */
float
computeCloudTop(
  const std::vector<float>& col,
  const std::vector<float>& hlevelsM)
{
  const int numhts = static_cast<int>(col.size());
  float h = Constants::DataUnavailable;

  for (int k = numhts - 1; k >= 0; --k) {
    const float val = col[k];
    if (val == Constants::DataUnavailable) {
      continue;
    }
    h = Constants::MissingData;
    if (Constants::isGood(val)) {
      h = hlevelsM[k] / 1000.0f;
      break;
    }
  }
  return h;
}

/** Extract value at a desired NSE height (meters). */
void
computeValueAtHeight(
  const std::vector<float>& col,
  const std::vector<float>& hlevelsM,
  float                   desiredHeightM,
  float                   & outVal,
  float                   & outHeightM)
{
  const int numhts = static_cast<int>(col.size());

  if (numhts <= 0) {
    outVal     = Constants::MissingData;
    outHeightM = Constants::MissingData;
    return;
  }

  int lower  = -1;
  int higher = -1;

  for (int k = 0; k < numhts - 1; ++k) {
    if ((hlevelsM[k] <= desiredHeightM) && (hlevelsM[k + 1] >= desiredHeightM) ) {
      lower  = k;
      higher = k + 1;
    }
  }
  if (lower < 0) {
    if (desiredHeightM < hlevelsM[0]) {
      lower  = 0;
      higher = 0;
    } else {
      lower  = numhts - 1;
      higher = numhts - 1;
    }
  }

  if (col[lower] > col[higher]) {
    outVal     = col[lower];
    outHeightM = hlevelsM[lower];
  } else {
    outVal     = col[higher];
    outHeightM = hlevelsM[higher];
  }
} // computeValueAtHeight

/** Compute depth (km) between echo top and echo bottom (or reference NSE height). */
float
computeDepth(
  const std::vector<float>& col,
  const std::vector<float>& hlevelsM,
  float                   threshold,
  bool                    hasRefHeight,
  float                   refHeightM)
{
  const float echoTop    = computeEchoTop(col, hlevelsM, threshold);
  const float echoBottom = hasRefHeight ?
    (refHeightM / 1000.0f) :
    computeEchoBottom(col, hlevelsM, threshold);

  if (Constants::isGood(echoTop)) {
    if (Constants::isGood(echoBottom)) {
      const float depth = echoTop - echoBottom;
      return (depth < 0.0f) ? Constants::MissingData : depth;
    }
    return Constants::MissingData;
  }
  return echoTop;
}

/** Resolve inclusive [lower, higher] layer indices for [minHeightM, maxHeightM]. */
void
resolveLayerBounds(
  const std::vector<float>& hlevelsM,
  float                   minHeightM,
  float                   maxHeightM,
  int                     & lower,
  int                     & higher)
{
  const int numhts = static_cast<int>(hlevelsM.size());

  lower = 0;
  while (lower < numhts && hlevelsM[lower] < minHeightM) {
    ++lower;
  }
  higher = numhts - 1;
  while (higher >= 0 && hlevelsM[higher] > maxHeightM) {
    --higher;
  }
}

void
computeLayerAvg(
  const std::vector<float>& col,
  const std::vector<float>& hlevelsM,
  float                   minHeightM,
  float                   maxHeightM,
  float                   & outVal,
  float                   & outHeightM)
{
  int lower = 0, higher = -1;

  resolveLayerBounds(hlevelsM, minHeightM, maxHeightM, lower, higher);

  float defaultVal = Constants::DataUnavailable;
  int numLayers    = 0;
  float sum        = 0.0f;
  float sumHt      = 0.0f;

  for (int k = lower; k <= higher; ++k) {
    const float c = col[k];
    if (c == Constants::MissingData) {
      defaultVal = Constants::MissingData;
      continue;
    }
    if (Constants::isGood(c)) {
      sum   += c;
      sumHt += hlevelsM[k];
      ++numLayers;
    }
  }

  if (numLayers > 0) {
    outVal     = sum / static_cast<float>(numLayers);
    outHeightM = sumHt / static_cast<float>(numLayers);
  } else {
    outVal     = defaultVal;
    outHeightM = defaultVal;
  }
}

void
computeLayerLowestAlt(
  const std::vector<float>& col,
  const std::vector<float>& hlevelsM,
  float                   minHeightM,
  float                   maxHeightM,
  float                   & outVal,
  float                   & outHeightM)
{
  int lower = 0, higher = -1;

  resolveLayerBounds(hlevelsM, minHeightM, maxHeightM, lower, higher);

  outVal     = Constants::DataUnavailable;
  outHeightM = Constants::DataUnavailable;

  for (int k = lower; k <= higher; ++k) {
    const float c = col[k];
    if (c == Constants::DataUnavailable) {
      continue;
    }
    outVal     = c;
    outHeightM = hlevelsM[k];
    break;
  }
}

void
computeLayerMax(
  const std::vector<float>& col,
  const std::vector<float>& hlevelsM,
  float                   minHeightM,
  float                   maxHeightM,
  float                   & outVal,
  float                   & outHeightM)
{
  int lower = 0, higher = -1;

  resolveLayerBounds(hlevelsM, minHeightM, maxHeightM, lower, higher);

  outVal     = Constants::DataUnavailable;
  outHeightM = Constants::DataUnavailable;

  for (int k = lower; k <= higher; ++k) {
    const float c = col[k];
    if (c == Constants::DataUnavailable) {
      continue;
    }
    if (outVal == Constants::DataUnavailable) {
      outVal     = Constants::MissingData;
      outHeightM = Constants::MissingData;
    }
    if (Constants::isGood(c) && (c > outVal) ) {
      outVal     = c;
      outHeightM = hlevelsM[k];
    }
  }
}

void
computeLayerMin(
  const std::vector<float>& col,
  const std::vector<float>& hlevelsM,
  float                   minHeightM,
  float                   maxHeightM,
  float                   & outVal,
  float                   & outHeightM)
{
  int lower = 0, higher = -1;

  resolveLayerBounds(hlevelsM, minHeightM, maxHeightM, lower, higher);

  outVal     = Constants::DataUnavailable;
  outHeightM = Constants::DataUnavailable;

  for (int k = lower; k <= higher; ++k) {
    const float c = col[k];
    if (c == Constants::DataUnavailable) {
      continue;
    }
    if (outVal == Constants::DataUnavailable) {
      outVal     = Constants::MissingData;
      outHeightM = Constants::MissingData;
    }
    if (Constants::isGood(c) && (!Constants::isGood(outVal) || (c < outVal) )) {
      outVal     = c;
      outHeightM = hlevelsM[k];
    }
  }
}
} // namespace

namespace rapio {
LayerAverageAlg::LayerAverageAlg()
  : VolumeAlgorithm("3DLayerAverage")
{ }

size_t
LayerAverageAlg::registerHeightSpec(const std::string& productName, float defaultMeters)
{
  for (size_t i = 0; i < myHeightSpecs.size(); ++i) {
    if ((myHeightSpecs[i].productName == productName) &&
      (myHeightSpecs[i].defaultMeters == defaultMeters) )
    {
      return i;
    }
  }
  myHeightSpecs.push_back({ productName, defaultMeters });
  return myHeightSpecs.size() - 1;
}

void
LayerAverageAlg::declareOptions(RAPIOOptions& o)
{
  VolumeAlgorithm::declareOptions(o);
  o.setDescription(
    "Calculates Isotherms and Layer-Averaged products configured via <layeraverage> XML blocks "
    "embedded in -iconfig (single or fusionalgs.xml) or via --config.");
  o.setAuthors("Robert Toomey, Valliappa Lakshmanan (Lak)");
  o.optional("config", "", "Optional path to XML file with <layeraverage> blocks (if not embedded in -iconfig)");
}

void
LayerAverageAlg::processOptions(RAPIOOptions& o)
{
  VolumeAlgorithm::processOptions(o);
  myConfigFile = o.getString("config");

  bool loaded = false;

  if (auto cfgNode = o.getConfigNode()) {
    loaded = loadConfigFromNode(*cfgNode);
  }
  if (!loaded && !myConfigFile.empty()) {
    loaded = loadConfigFromFile(myConfigFile);
  }
  if (!loaded || myLayerGroups.empty()) {
    throw StartupException(
            "3DLayerAverage: No <layeraverage> blocks found. Provide <layeraverage> elements via "
            "-iconfig (standalone or fusionalgs.xml) or --config.");
  }
}

bool
LayerAverageAlg::loadConfigFromFile(const std::string& path)
{
  RAPIORuntime::initialize();
  auto ptreeData = IODataType::read<PTreeData>(path, "xml");

  if (!ptreeData) {
    ptreeData = Config::huntXML(path);
  }
  if (!ptreeData) {
    fLogSevere("3DLayerAverage: Could not load XML config '{}'.", path);
    return false;
  }
  return loadConfigFromPTree(ptreeData);
}

bool
LayerAverageAlg::loadConfigFromXMLString(const std::string& xmlContent)
{
  RAPIORuntime::initialize();
  std::vector<char> buf(xmlContent.begin(), xmlContent.end());
  auto ptreeData = IODataType::readBuffer<PTreeData>(buf, "xml");

  if (!ptreeData) {
    fLogSevere("3DLayerAverage: Failed to parse XML configuration string.");
    return false;
  }
  return loadConfigFromPTree(ptreeData);
}

bool
LayerAverageAlg::parseOperationNode(const PTreeNode& node, OperationConfig& outOp)
{
  outOp.productName       = node.getAttr("name", std::string(""));
  outOp.heightProductName = node.getAttr("outputheight", std::string(""));

  const auto heightNodes = node.getChildren("height");
  std::vector<size_t> heightIndices;

  heightIndices.reserve(heightNodes.size());
  for (const auto& hNode : heightNodes) {
    const std::string prod = hNode.getAttr("product", std::string(""));
    const float defVal     = hNode.getAttr("default", 0.0f);
    heightIndices.push_back(registerHeightSpec(prod, defVal));
  }

  const std::string opStr = node.getAttr("operation", std::string(""));

  if (opStr == "at") {
    if (heightIndices.size() != 1) {
      fLogSevere("3DLayerAverage: 'at' operation for '{}' requires exactly 1 <height> element.", outOp.productName);
      return false;
    }
    outOp.type         = OpType::At;
    outOp.heightIndex1 = static_cast<int>(heightIndices[0]);
    return true;
  }

  if (opStr == "echotop") {
    const std::string threshStr = node.getAttr("threshold", std::string(""));
    if (threshStr.empty()) {
      fLogSevere("3DLayerAverage: 'echotop' operation for '{}' requires a threshold attribute.", outOp.productName);
      return false;
    }
    outOp.type      = OpType::EchoTop;
    outOp.threshold = std::stof(threshStr);
    outOp.heightProductName.clear();
    return true;
  }

  if (opStr == "cloudtop") {
    outOp.type = OpType::CloudTop;
    outOp.heightProductName.clear();
    return true;
  }

  if (opStr == "depth") {
    const std::string threshStr = node.getAttr("threshold", std::string(""));
    if (threshStr.empty()) {
      fLogSevere("3DLayerAverage: 'depth' operation for '{}' requires a threshold attribute.", outOp.productName);
      return false;
    }
    if (heightIndices.size() > 1) {
      fLogSevere("3DLayerAverage: 'depth' operation for '{}' accepts 0 or 1 <height> elements.", outOp.productName);
      return false;
    }
    outOp.type         = OpType::Depth;
    outOp.threshold    = std::stof(threshStr);
    outOp.heightIndex1 = heightIndices.empty() ? -1 : static_cast<int>(heightIndices[0]);
    outOp.heightProductName.clear();
    return true;
  }

  if (heightIndices.size() != 2) {
    fLogSevere("3DLayerAverage: '{}' operation for '{}' requires exactly 2 <height> elements.", opStr,
      outOp.productName);
    return false;
  }

  outOp.heightIndex1 = static_cast<int>(heightIndices[0]);
  outOp.heightIndex2 = static_cast<int>(heightIndices[1]);

  if (opStr == "avg") {
    outOp.type = OpType::Avg;
    return true;
  }
  if (opStr == "lowestalt") {
    outOp.type = OpType::LowestAlt;
    return true;
  }
  if (opStr == "max") {
    outOp.type = OpType::Max;
    return true;
  }
  if (opStr == "min") {
    outOp.type = OpType::Min;
    return true;
  }

  fLogSevere("3DLayerAverage: Unknown operation '{}'.", opStr);
  return false;
} // LayerAverageAlg::parseOperationNode

bool
LayerAverageAlg::loadConfigFromPTree(std::shared_ptr<PTreeData> ptreeData)
{
  if (!ptreeData || !ptreeData->getTree()) {
    return false;
  }
  return loadConfigFromNode(*ptreeData->getTree());
}

bool
LayerAverageAlg::loadConfigFromNode(const PTreeNode& node)
{
  myLayerGroups.clear();
  myHeightSpecs.clear();

  std::vector<PTreeNode> layerNodes = node.getChildren("layeraverage");

  if (layerNodes.empty()) {
    const std::string firstChild = node.getFirstChildName();
    if (!firstChild.empty()) {
      if (auto docRoot = node.getChildOptional(firstChild)) {
        layerNodes = docRoot->getChildren("layeraverage");
        // Also check nested <w2algxml program="LayerAverage"> inside a fusionalgs.xml
        if (layerNodes.empty()) {
          for (const auto& childAlg : docRoot->getChildren("w2algxml")) {
            auto nested = childAlg.getChildren("layeraverage");
            layerNodes.insert(layerNodes.end(), nested.begin(), nested.end());
          }
        }
      }
    }
  }

  if (layerNodes.empty()) {
    fLogSevere("3DLayerAverage: No <layeraverage> elements found in XML configuration.");
    return false;
  }

  for (const auto& laNode : layerNodes) {
    LayerGroupConfig group;
    group.nameOf3DGrid = laNode.getAttr("nameof3dgrid", std::string(""));
    group.unit         = laNode.getAttr("unit", std::string("dimensionless"));
    group.colormap     = laNode.getAttr("colormap", std::string("Default"));

    const auto prodNodes = laNode.getChildren("product");
    for (const auto& pNode : prodNodes) {
      OperationConfig op;
      if (parseOperationNode(pNode, op)) {
        if (!op.productName.empty()) {
          declareProduct(op.productName, "LayerAverage product");
        }
        if (!op.heightProductName.empty()) {
          declareProduct(op.heightProductName, "LayerAverage height product");
        }
        group.operations.push_back(std::move(op));
      }
    }
    myLayerGroups.push_back(std::move(group));
  }

  return !myLayerGroups.empty();
} // LayerAverageAlg::loadConfigFromNode

void
LayerAverageAlg::checkOutputGrids(std::shared_ptr<LatLonHeightGrid> input)
{
  const Time forTime = input->getTime();

  myCurrentInputTypeName = input->getTypeName();
  const bool coverageChanged = checkCoverageChange(input);
  const auto coverage        = input->getLLCoverageArea();

  if (coverageChanged) {
    fLogInfo("Coverage changed, updating LayerAverage output grids...");
  }

  for (auto& group : myLayerGroups) {
    if (!wildcardMatches(group.nameOf3DGrid, myCurrentInputTypeName)) {
      continue;
    }
    for (auto& op : group.operations) {
      if (!op.resultGrid || coverageChanged) {
        op.resultGrid = LatLonGrid::Create(op.productName, group.unit, forTime, coverage);
        op.resultGrid->setDataAttributeValue("SubType", "");
        op.resultGrid->setDataAttributeValue("ColorMap", group.colormap);
      }
      op.resultGrid->setTime(forTime);

      if (!op.heightProductName.empty()) {
        if (!op.heightGrid || coverageChanged) {
          op.heightGrid = LatLonGrid::Create(op.heightProductName, "m", forTime, coverage);
          op.heightGrid->setDataAttributeValue("SubType", "");
          op.heightGrid->setDataAttributeValue("ColorMap", "Height");
        }
        op.heightGrid->setTime(forTime);
      }
    }
  }
} // LayerAverageAlg::checkOutputGrids

class LayerAverageCallback : public LatLonHeightGridCallback {
public:
  struct ResolvedHeight {
    std::shared_ptr<DataProjection> projection;
    float                           defaultMeters = 0.0f;
  };

  struct ActiveOp {
    LayerAverageAlg::OpType type;
    float                   threshold    = 0.0f;
    int                     heightIndex1 = -1;
    int                     heightIndex2 = -1;
    ArrayFloat2DPtr         resultOut;
    ArrayFloat2DPtr         heightOut;
  };

  LayerAverageCallback(
    std::vector<ResolvedHeight> heights,
    std::vector<ActiveOp>       ops)
    : myResolvedHeights(std::move(heights)),
    myOps(std::move(ops)){ }

  void
  handleBeginLoop(LatLonHeightGridIterator * /*it*/, const LatLonHeightGrid& grid) override
  {
    const size_t numLayers = grid.getNumLayers();

    myHLevelsM.resize(numLayers);
    for (size_t k = 0; k < numLayers; ++k) {
      myHLevelsM[k] = grid.getLayerValue(k);
    }
    myColValues.assign(numLayers, Constants::DataUnavailable);
    myColHeightsM.resize(myResolvedHeights.size());
  }

  void
  handleBeginColumn(LatLonHeightGridIterator * it) override
  {
    std::fill(myColValues.begin(), myColValues.end(), Constants::DataUnavailable);

    const float atLat = it->getCurrentLatDegs();
    const float atLon = it->getCurrentLonDegs();

    for (size_t idx = 0; idx < myResolvedHeights.size(); ++idx) {
      float hM = myResolvedHeights[idx].defaultMeters;
      if (myResolvedHeights[idx].projection) {
        const float val = static_cast<float>(myResolvedHeights[idx].projection->getValueAtLL(atLat, atLon));
        if (Constants::isGood(val)) {
          hM = val;
        }
      }
      myColHeightsM[idx] = hM;
    }
  }

  void
  handleVoxel(LatLonHeightGridIterator * it) override
  {
    const size_t z = it->getCurrentLayerIdx();

    if (z < myColValues.size()) {
      myColValues[z] = it->getValue();
    }
  }

  void
  handleEndColumn(LatLonHeightGridIterator * it) override
  {
    const size_t i = it->getCurrentLatIdx();
    const size_t j = it->getCurrentLonIdx();

    for (const auto& op : myOps) {
      float val = Constants::DataUnavailable;
      float htM = Constants::DataUnavailable;

      switch (op.type) {
          case LayerAverageAlg::OpType::At:
            computeValueAtHeight(myColValues, myHLevelsM, myColHeightsM[op.heightIndex1], val, htM);
            break;
          case LayerAverageAlg::OpType::EchoTop:
            val = computeEchoTop(myColValues, myHLevelsM, op.threshold);
            break;
          case LayerAverageAlg::OpType::CloudTop:
            val = computeCloudTop(myColValues, myHLevelsM);
            break;
          case LayerAverageAlg::OpType::Depth:
            val = computeDepth(
              myColValues, myHLevelsM, op.threshold,
              op.heightIndex1 >= 0,
              (op.heightIndex1 >= 0) ? myColHeightsM[op.heightIndex1] : 0.0f);
            break;
          case LayerAverageAlg::OpType::Avg:
            computeLayerAvg(myColValues, myHLevelsM, myColHeightsM[op.heightIndex1], myColHeightsM[op.heightIndex2],
              val, htM);
            break;
          case LayerAverageAlg::OpType::LowestAlt:
            computeLayerLowestAlt(myColValues, myHLevelsM, myColHeightsM[op.heightIndex1],
              myColHeightsM[op.heightIndex2], val, htM);
            break;
          case LayerAverageAlg::OpType::Max:
            computeLayerMax(myColValues, myHLevelsM, myColHeightsM[op.heightIndex1], myColHeightsM[op.heightIndex2],
              val, htM);
            break;
          case LayerAverageAlg::OpType::Min:
            computeLayerMin(myColValues, myHLevelsM, myColHeightsM[op.heightIndex1], myColHeightsM[op.heightIndex2],
              val, htM);
            break;
      }

      if (op.resultOut) {
        (*op.resultOut)[i][j] = val;
      }
      if (op.heightOut) {
        (*op.heightOut)[i][j] = htM;
      }
    }
  } // handleEndColumn

private:
  std::vector<ResolvedHeight> myResolvedHeights;
  std::vector<ActiveOp> myOps;
  std::vector<float> myHLevelsM;
  std::vector<float> myColValues;
  std::vector<float> myColHeightsM;
};

std::unique_ptr<LatLonHeightGridCallback>
LayerAverageAlg::createCallback()
{
  std::vector<LayerAverageCallback::ResolvedHeight> resolvedHeights;

  resolvedHeights.reserve(myHeightSpecs.size());
  for (const auto& spec : myHeightSpecs) {
    resolvedHeights.push_back({ getNSEProjection(spec.productName), spec.defaultMeters });
  }

  std::vector<LayerAverageCallback::ActiveOp> activeOps;

  for (const auto& group : myLayerGroups) {
    if (!wildcardMatches(group.nameOf3DGrid, myCurrentInputTypeName)) {
      continue;
    }
    for (const auto& op : group.operations) {
      if (!op.resultGrid) {
        continue;
      }
      LayerAverageCallback::ActiveOp active;
      active.type         = op.type;
      active.threshold    = op.threshold;
      active.heightIndex1 = op.heightIndex1;
      active.heightIndex2 = op.heightIndex2;
      active.resultOut    = op.resultGrid->getFloat2DPtr();
      active.heightOut    = op.heightGrid ? op.heightGrid->getFloat2DPtr() : nullptr;
      activeOps.push_back(std::move(active));
    }
  }

  return std::make_unique<LayerAverageCallback>(std::move(resolvedHeights), std::move(activeOps));
}

void
LayerAverageAlg::writeFinalProducts(RAPIOAlgorithm * writer)
{
  if (!writer) {
    return;
  }
  for (const auto& group : myLayerGroups) {
    if (!wildcardMatches(group.nameOf3DGrid, myCurrentInputTypeName)) {
      continue;
    }
    for (const auto& op : group.operations) {
      if (op.resultGrid) {
        writer->writeOutputProduct(op.productName, op.resultGrid);
      }
      if (op.heightGrid && !op.heightProductName.empty()) {
        writer->writeOutputProduct(op.heightProductName, op.heightGrid);
      }
    }
  }
}
} // namespace rapio
