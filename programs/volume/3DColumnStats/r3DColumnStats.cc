#include "r3DColumnStats.h"
#include "rIODataType.h"
#include "rLatLonHeightGridIterator.h"
#include "rConstants.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <sstream>

using namespace rapio;

extern "C" {
RAPIOAlgorithm *
createRAPIOAlg(void)
{
  return reinterpret_cast<RAPIOAlgorithm *>(new rapio::ColumnStatsAlg());
}
}

namespace {
std::string
trimAndLower(const std::string& s)
{
  size_t start = 0;

  while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
    ++start;
  }
  size_t end = s.size();

  while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
    --end;
  }
  std::string out = s.substr(start, end - start);

  std::transform(out.begin(), out.end(), out.begin(),
    [](unsigned char c) {
      return static_cast<char>(std::tolower(c));
    });
  return out;
}

void
copyColorMapIfPresent(
  const std::shared_ptr<LatLonHeightGrid>& input,
  const std::shared_ptr<LatLonGrid>      & output)
{
  std::string colorMap;

  if (input && output && input->getString("ColorMap-value", colorMap)) {
    output->setDataAttributeValue("ColorMap", colorMap);
  }
}
} // namespace

namespace rapio {
ColumnStatsAlg::ColumnStatsAlg()
  : VolumeAlgorithm("3DColumnStats")
{ }

void
ColumnStatsAlg::declareOptions(RAPIOOptions& o)
{
  VolumeAlgorithm::declareOptions(o);
  o.setDescription(
    "Single-pass 3D column statistics consolidating legacy w2merger vertical reductions "
    "(VerticalMaximum, VerticalMinimum, VerticalAbsMax, VerticalAverage, VerticalDepth, LowHtRefl).");
  o.setAuthors("Robert Toomey, Valliappa Lakshmanan (Lak)");
  o.optional("ops", "max",
    "Comma-separated operations to compute: max,min,absmax,avg,depth,lowht,all");
  o.optional("threshold", "55.0",
    "Threshold value for 'depth' (vol_VerticalDepth) and 'lowht' (vol_LowHtRefl)");
  o.optional("bottomaglkm", "0.0",
    "Optional bottom height AGL in km (used when --topaglkm > 0; default 0.0)");
  o.optional("topaglkm", "-1.0",
    "Optional AGL ceiling in km (> 0 caps column to [terrain+bottomaglkm, terrain+topaglkm]; <= 0 scans full column)");
}

void
ColumnStatsAlg::processOptions(RAPIOOptions& o)
{
  VolumeAlgorithm::processOptions(o);
  myThreshold    = o.getFloat("threshold");
  myBottomAglKMs = o.getFloat("bottomaglkm");
  myTopAglKMs    = o.getFloat("topaglkm");
  parseOpsString(o.getString("ops"));
}

bool
ColumnStatsAlg::parseOpsString(const std::string& opsStr)
{
  myDoMax    = false;
  myDoMin    = false;
  myDoAbsMax = false;
  myDoAvg    = false;
  myDoDepth  = false;
  myDoLowHt  = false;

  bool anyValid = false;
  std::stringstream ss(opsStr);
  std::string token;

  while (std::getline(ss, token, ',')) {
    const std::string op = trimAndLower(token);
    if (op.empty()) {
      continue;
    }
    if (op == "all") {
      myDoMax  = myDoMin = myDoAbsMax = myDoAvg = myDoDepth = myDoLowHt = true;
      anyValid = true;
    } else if (op == "max") {
      myDoMax  = true;
      anyValid = true;
    } else if (op == "min") {
      myDoMin  = true;
      anyValid = true;
    } else if (op == "absmax") {
      myDoAbsMax = true;
      anyValid   = true;
    } else if ((op == "avg") || (op == "average") ) {
      myDoAvg  = true;
      anyValid = true;
    } else if (op == "depth") {
      myDoDepth = true;
      anyValid  = true;
    } else if ((op == "lowht") || (op == "lowhtrefl") ) {
      myDoLowHt = true;
      anyValid  = true;
    } else {
      fLogSevere("3DColumnStats: Unknown operation '{}' in --ops.", op);
    }
  }

  if (myDoMax)    { declareProduct("VerticalMaximum", "Column maximum (legacy vol_VerticalMaximum)"); }
  if (myDoMin)    { declareProduct("VerticalMinimum", "Column minimum (legacy vol_VerticalMinimum)"); }
  if (myDoAbsMax) {
    declareProduct("VerticalAbsMax", "Column absolute maximum retaining sign (legacy vol_VerticalAbsMax)");
  }
  if (myDoAvg)    { declareProduct("VerticalAverage", "Column average (legacy vol_VerticalAverage)"); }
  if (myDoDepth)  { declareProduct("VerticalDepth", "Layer count >= threshold (legacy vol_VerticalDepth)"); }
  if (myDoLowHt)  { declareProduct("LowHtRefl", "Lowest layer index >= threshold (legacy vol_LowHtRefl)"); }

  return anyValid;
} // ColumnStatsAlg::parseOpsString

void
ColumnStatsAlg::checkOutputGrids(std::shared_ptr<LatLonHeightGrid> input)
{
  const Time forTime        = input->getTime();
  const auto coverage       = input->getLLCoverageArea();
  const std::string inName  = input->getTypeName();
  const std::string inUnits = input->getUnits();

  const bool needRebuild = checkCoverageChange(input) || (myCachedInputTypeName != inName);

  myCachedInputTypeName = inName;

  const bool useAgl = (myTopAglKMs > 0.0f);
  std::string subType;

  if (useAgl) {
    char subTypeBuf[32];
    if (myBottomAglKMs == 0.0f) {
      // Exact SubType format match with retired 3DMaxAGL (e.g. "0-4.0km")
      std::snprintf(subTypeBuf, sizeof(subTypeBuf), "0-%.1fkm", myTopAglKMs);
    } else {
      std::snprintf(subTypeBuf, sizeof(subTypeBuf), "%.1f-%.1fkm", myBottomAglKMs, myTopAglKMs);
    }
    subType = subTypeBuf;
  }

  // --- max: legacy w2merger/vol_VerticalMaximum (or retired 3DMaxAGL when AGL-capped) ---
  if (myDoMax) {
    if (!myMaxGrid || needRebuild) {
      const std::string prodName = useAgl ? "MaxAGL" : (inName + "Composite");
      myMaxGrid = LatLonGrid::Create(prodName, inUnits, forTime, coverage);
      myMaxGrid->setDataAttributeValue("SubType", subType);
      copyColorMapIfPresent(input, myMaxGrid);
    }
    myMaxGrid->setTime(forTime);
  }

  // --- min: legacy w2merger/vol_VerticalMinimum (or MinAGL when AGL-capped) ---
  if (myDoMin) {
    if (!myMinGrid || needRebuild) {
      const std::string prodName = useAgl ? "MinAGL" : (inName + "CompositeMin");
      myMinGrid = LatLonGrid::Create(prodName, inUnits, forTime, coverage);
      myMinGrid->setDataAttributeValue("SubType", subType);
      copyColorMapIfPresent(input, myMinGrid);
    }
    myMinGrid->setTime(forTime);
  }

  // --- absmax: legacy w2merger/vol_VerticalAbsMax (or AbsMaxAGL when AGL-capped) ---
  if (myDoAbsMax) {
    if (!myAbsMaxGrid || needRebuild) {
      const std::string prodName = useAgl ? "AbsMaxAGL" : (inName + "CompositeAbsMax");
      myAbsMaxGrid = LatLonGrid::Create(prodName, inUnits, forTime, coverage);
      myAbsMaxGrid->setDataAttributeValue("SubType", subType);
      copyColorMapIfPresent(input, myAbsMaxGrid);
    }
    myAbsMaxGrid->setTime(forTime);
  }

  // --- avg: legacy w2merger/vol_VerticalAverage (or AvgAGL when AGL-capped) ---
  if (myDoAvg) {
    if (!myAvgGrid || needRebuild) {
      const std::string prodName = useAgl ? "AvgAGL" : (inName + "Average");
      myAvgGrid = LatLonGrid::Create(prodName, inUnits, forTime, coverage);
      myAvgGrid->setDataAttributeValue("SubType", subType);
      copyColorMapIfPresent(input, myAvgGrid);
    }
    myAvgGrid->setTime(forTime);
  }

  // --- depth: legacy w2merger/vol_VerticalDepth (or DepthAGL when AGL-capped) ---
  if (myDoDepth) {
    if (!myDepthGrid || needRebuild) {
      const std::string prodName = useAgl ? "DepthAGL" : (inName + "Depth");
      myDepthGrid = LatLonGrid::Create(prodName, "layers", forTime, coverage);
      myDepthGrid->setDataAttributeValue("SubType", subType);
    }
    myDepthGrid->setTime(forTime);
  }

  // --- lowht: legacy w2merger/vol_LowHtRefl (or LowHtAGL when AGL-capped) ---
  if (myDoLowHt) {
    if (!myLowHtGrid || needRebuild) {
      const std::string prodName = useAgl ? "LowHtAGL" : "LowHtRefl";
      myLowHtGrid = LatLonGrid::Create(prodName, "layer", forTime, coverage);
      myLowHtGrid->setDataAttributeValue("SubType", subType);
    }
    myLowHtGrid->setTime(forTime);
  }
} // ColumnStatsAlg::checkOutputGrids

/**
 * @class ColumnStatsCallback
 * @brief Streaming single-pass column callback (IterateMode::ColumnsDown).
 *
 * Each enabled legacy w2merger reduction updates scalar accumulators in handleVoxel()
 * and writes its 2D output pixel in handleEndColumn().
 */
class ColumnStatsCallback : public LatLonHeightGridCallback {
public:
  ColumnStatsCallback(
    bool doMax, bool doMin, bool doAbsMax, bool doAvg, bool doDepth, bool doLowHt,
    float threshold, float bottomAglKMs, float topAglKMs,
    ArrayFloat2DPtr maxOut,
    ArrayFloat2DPtr minOut,
    ArrayFloat2DPtr absMaxOut,
    ArrayFloat2DPtr avgOut,
    ArrayFloat2DPtr depthOut,
    ArrayFloat2DPtr lowHtOut)
    : myDoMax(doMax),
    myDoMin(doMin),
    myDoAbsMax(doAbsMax),
    myDoAvg(doAvg),
    myDoDepth(doDepth),
    myDoLowHt(doLowHt),
    myThreshold(threshold),
    myBottomAglKMs(bottomAglKMs),
    myTopAglKMs(topAglKMs),
    myUseAglWindow(topAglKMs > 0.0f),
    myMaxOut(std::move(maxOut)),
    myMinOut(std::move(minOut)),
    myAbsMaxOut(std::move(absMaxOut)),
    myAvgOut(std::move(avgOut)),
    myDepthOut(std::move(depthOut)),
    myLowHtOut(std::move(lowHtOut)){ }

  void
  handleBeginColumn(LatLonHeightGridIterator * it) override
  {
    myDataAvail = false;
    myHasGood   = false;

    // Reset scalar accumulators
    myMaxVal     = 0.0f;
    myMinVal     = 0.0f;
    myAbsMaxVal  = 0.0f;
    mySum        = 0.0f;
    myCount      = 0;
    myDepthCount = 0;
    myLowHtIdx   = 0;

    if (myUseAglWindow) {
      const float terrainHeightKMs = it->getTerrainHeightKMs();
      myBottomCutoffKMs = terrainHeightKMs + myBottomAglKMs;
      myTopCutoffKMs    = terrainHeightKMs + myTopAglKMs;
    }
  }

  void
  handleVoxel(LatLonHeightGridIterator * it) override
  {
    if (myUseAglWindow) {
      const float currentHtKMs = it->getCurrentHeightKMs();
      if ((currentHtKMs < myBottomCutoffKMs) || (currentHtKMs > myTopCutoffKMs) ) {
        return;
      }
    }

    const float v = it->getValue();

    if (v == Constants::DataUnavailable) {
      return;
    }
    myDataAvail = true;

    if (!Constants::isGood(v)) {
      return;
    }

    // 1. max (legacy w2merger/vol_VerticalMaximum)
    if (myDoMax) {
      if (!myHasGood || (v > myMaxVal) ) {
        myMaxVal = v;
      }
    }

    // 2. min (legacy w2merger/vol_VerticalMinimum)
    if (myDoMin) {
      if (!myHasGood || (v < myMinVal) ) {
        myMinVal = v;
      }
    }

    // 3. absmax (legacy w2merger/vol_VerticalAbsMax — max magnitude, retains sign)
    if (myDoAbsMax) {
      if (!myHasGood || (std::fabs(v) > std::fabs(myAbsMaxVal))) {
        myAbsMaxVal = v;
      }
    }

    // 4. avg (legacy w2merger/vol_VerticalAverage)
    if (myDoAvg) {
      mySum += v;
      ++myCount;
    }

    // 5. depth (legacy w2merger/vol_VerticalDepth — count of layers >= threshold)
    if (myDoDepth && (v >= myThreshold) ) {
      ++myDepthCount;
    }

    // 6. lowht (legacy w2merger/vol_LowHtRefl — lowest 1-based layer index >= threshold)
    // Because we scan ColumnsDown (top-to-bottom), overwriting on each hit naturally
    // leaves the lowest layer index that met the threshold.
    if (myDoLowHt && (v >= myThreshold) ) {
      myLowHtIdx = static_cast<int>(it->getCurrentLayerIdx()) + 1;
    }

    myHasGood = true;
  } // handleVoxel

  void
  handleEndColumn(LatLonHeightGridIterator * it) override
  {
    const size_t i       = it->getCurrentLatIdx();
    const size_t j       = it->getCurrentLonIdx();
    const float fallback = myDataAvail ? Constants::MissingData : Constants::DataUnavailable;

    // vol_VerticalMaximum
    if (myDoMax && myMaxOut) {
      (*myMaxOut)[i][j] = myHasGood ? myMaxVal : fallback;
    }

    // vol_VerticalMinimum
    if (myDoMin && myMinOut) {
      (*myMinOut)[i][j] = myHasGood ? myMinVal : fallback;
    }

    // vol_VerticalAbsMax
    if (myDoAbsMax && myAbsMaxOut) {
      (*myAbsMaxOut)[i][j] = myHasGood ? myAbsMaxVal : fallback;
    }

    // vol_VerticalAverage
    if (myDoAvg && myAvgOut) {
      (*myAvgOut)[i][j] = (myCount > 0) ? (mySum / static_cast<float>(myCount)) : fallback;
    }

    // vol_VerticalDepth
    if (myDoDepth && myDepthOut) {
      (*myDepthOut)[i][j] = (myDepthCount > 0) ? static_cast<float>(myDepthCount) : fallback;
    }

    // vol_LowHtRefl
    if (myDoLowHt && myLowHtOut) {
      (*myLowHtOut)[i][j] = (myLowHtIdx > 0) ? static_cast<float>(myLowHtIdx) : fallback;
    }
  } // handleEndColumn

private:
  const bool myDoMax;
  const bool myDoMin;
  const bool myDoAbsMax;
  const bool myDoAvg;
  const bool myDoDepth;
  const bool myDoLowHt;

  const float myThreshold;
  const float myBottomAglKMs;
  const float myTopAglKMs;
  const bool myUseAglWindow;

  ArrayFloat2DPtr myMaxOut;
  ArrayFloat2DPtr myMinOut;
  ArrayFloat2DPtr myAbsMaxOut;
  ArrayFloat2DPtr myAvgOut;
  ArrayFloat2DPtr myDepthOut;
  ArrayFloat2DPtr myLowHtOut;

  // Per-column streaming state
  bool myDataAvail        = false;
  bool myHasGood          = false;
  float myMaxVal          = 0.0f;
  float myMinVal          = 0.0f;
  float myAbsMaxVal       = 0.0f;
  float mySum             = 0.0f;
  int myCount             = 0;
  int myDepthCount        = 0;
  int myLowHtIdx          = 0;
  float myBottomCutoffKMs = 0.0f;
  float myTopCutoffKMs    = 0.0f;
};

std::unique_ptr<LatLonHeightGridCallback>
ColumnStatsAlg::createCallback()
{
  if ((myTopAglKMs > 0.0f) && !haveTerrain() && !myWarnedTerrain) {
    fLogSevere("3DColumnStats: --topaglkm set without terrain; defaulting terrain height to 0.0 km (sea level).");
    myWarnedTerrain = true;
  }

  if ((myTopAglKMs > 0.0f) && (myBottomAglKMs > myTopAglKMs)) {
    fLogSevere("3DColumnStats: --bottomaglkm ({}) exceeds --topaglkm ({}); AGL window is empty.",
      myBottomAglKMs, myTopAglKMs);
  }

  if (!myDoMax && !myDoMin && !myDoAbsMax && !myDoAvg && !myDoDepth && !myDoLowHt) {
    fLogSevere("3DColumnStats: No operations enabled. Specify at least one op via --ops.");
    return nullptr;
  }

  return std::make_unique<ColumnStatsCallback>(
    myDoMax, myDoMin, myDoAbsMax, myDoAvg, myDoDepth, myDoLowHt,
    myThreshold, myBottomAglKMs, myTopAglKMs,
    myMaxGrid ? myMaxGrid->getFloat2DPtr() : nullptr,
    myMinGrid ? myMinGrid->getFloat2DPtr() : nullptr,
    myAbsMaxGrid ? myAbsMaxGrid->getFloat2DPtr() : nullptr,
    myAvgGrid ? myAvgGrid->getFloat2DPtr() : nullptr,
    myDepthGrid ? myDepthGrid->getFloat2DPtr() : nullptr,
    myLowHtGrid ? myLowHtGrid->getFloat2DPtr() : nullptr
  );
}

void
ColumnStatsAlg::writeFinalProducts(RAPIOAlgorithm * writer)
{
  if (!writer) {
    return;
  }
  if (myDoMax && myMaxGrid) {
    writer->writeOutputProduct(myMaxGrid->getTypeName(), myMaxGrid);
  }
  if (myDoMin && myMinGrid) {
    writer->writeOutputProduct(myMinGrid->getTypeName(), myMinGrid);
  }
  if (myDoAbsMax && myAbsMaxGrid) {
    writer->writeOutputProduct(myAbsMaxGrid->getTypeName(), myAbsMaxGrid);
  }
  if (myDoAvg && myAvgGrid) {
    writer->writeOutputProduct(myAvgGrid->getTypeName(), myAvgGrid);
  }
  if (myDoDepth && myDepthGrid) {
    writer->writeOutputProduct(myDepthGrid->getTypeName(), myDepthGrid);
  }
  if (myDoLowHt && myLowHtGrid) {
    writer->writeOutputProduct(myLowHtGrid->getTypeName(), myLowHtGrid);
  }
}
} // namespace rapio
