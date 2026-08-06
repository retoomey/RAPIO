#include "rWindResolver.h"
#include "rBinaryIO.h"
#include "rError.h"
#include "rColorTerm.h"
#include <cmath>

using namespace rapio;

// ============================================================================
// STAGE 2 STORAGE IMPLEMENTATION (Handles RLE)
// ============================================================================

WindStage2Storage::WindStage2Storage(
  const std::string   & radarName,
  const std::string   & typeName,
  const std::string   & units,
  const LLH           & center,
  const LLCoverageArea& radarGrid,
  const bool          noMissingSet) :
  myDimensions({ radarGrid.getNumX(), radarGrid.getNumY(), radarGrid.getNumZ() }),
  myMissingSet(myDimensions),
  myAddValueCounter(0), myAddMissingCounter(0)
{
  myTable = std::make_shared<WindBinaryTable>();
  if (noMissingSet) { myTable->setUseMissingAsUnavailable(); }
  myTable->setString("Sourcename", radarName);
  myTable->setString("Radarname", radarName);
  myTable->setString("Typename", typeName);
  myTable->setUnits(units);
  myTable->setLocation(center);
  myTable->setLong("xBase", radarGrid.getStartX());
  myTable->setLong("yBase", radarGrid.getStartY());
}

void
WindStage2Storage::RLE()
{
  for (size_t z = 0; z < myDimensions[2]; z++) {
    for (size_t y = 0; y < myDimensions[1]; y++) {
      for (size_t x = 0; x < myDimensions[0]; x++) {
        auto flag = myMissingSet.get13D(x, y, z);
        if (flag) {
          size_t startx = x;
          size_t starty = y;
          size_t startz = z;
          size_t length = 1;
          while (x + 1 < myDimensions[0]) {
            if (myMissingSet.get13D(x + 1, y, z)) {
              ++x;
              ++length;
            } else {
              break;
            }
          }
          myTable->addMissing(startx, starty, startz, length);
        }
      }
    }
  }
}

void
WindStage2Storage::send(RAPIOAlgorithm * alg, Time aTime, const std::string& asName)
{
  RLE(); // Compress the missing bitset into RLE vectors

  const size_t finalSize  = myTable->getValueSize();
  const size_t finalSize2 = myTable->getMissingSize();

  if ((finalSize < 1) && (finalSize2 < 1)) {
    fLogInfo("Skipping writing {} since we have 0 values.", mySubFolder);
    return;
  }

  fLogInfo("Writing Wind Data: {} values, {} missing as {} (RLE)", finalSize, myAddMissingCounter, finalSize2);
  
  IOConfig extraParams;
  extraParams.set("showfilesize", "yes");
  extraParams.set("outputsubfolder", mySubFolder);

  if (alg->isProductWanted("S2")) {
    myTable->setSubType("S2");
    myTable->setTime(aTime);
    myTable->setTypeName(asName);
    
    // Writes the .raw binary table
    alg->writeOutputProduct("S2", myTable, extraParams);
  }
}

// ============================================================================
// VOLUME VALUE IO IMPLEMENTATION
// ============================================================================

bool
WindVolumeValueIO::initForSend(
  const std::string    & radarName,
  const std::string    & typeName,
  const std::string    & units,
  const LLH            & center,
  const bool           noMissingSet,
  const PartitionInfo  & partition,
  const LLCoverageArea & radarGrid)
{
  for (size_t i = 0; i < partition.size(); ++i) {
    auto newOne = std::make_shared<WindStage2Storage>(radarName, typeName, units, center, radarGrid, noMissingSet);
    if (partition.getPartitionType() == PartitionInfo::Type::tile) {
      newOne->setSubFolder("partition" + std::to_string(i + 1));
    }
    myStorage.push_back(newOne);
  }
  return true;
}

void
WindVolumeValueIO::add(VolumeValue * vvp, short x, short y, short z, size_t partIndex)
{
  if (partIndex < myStorage.size()) {
    myStorage[partIndex]->add(vvp, x, y, z);
  }
}

void
WindVolumeValueIO::send(RAPIOAlgorithm * alg, Time aTime, const std::string& asName)
{
  fLogInfo("{}{}---Outputting Wind Data---{}", ColorTerm::green(), ColorTerm::bold(), ColorTerm::reset());
  for (auto& s : myStorage) {
    s->send(alg, aTime, asName);
  }
}

// ============================================================================
// WIND RESOLVER IMPLEMENTATION
// ============================================================================

void
WindResolver::introduceSelf()
{
  std::shared_ptr<WindResolver> newOne = std::make_shared<WindResolver>();
  Factory<VolumeValueResolver>::introduce("wind", newOne);
}

std::string
WindResolver::getHelpString(const std::string& fkey)
{
  return "Calculates the 5 geometric/velocity components required for Multi-Doppler Least Squares wind retrieval.";
}

std::shared_ptr<VolumeValueResolver>
WindResolver::create(const std::string & params)
{
  return std::make_shared<WindResolver>();
}

namespace {
// Helper function to extract and validate the single best tilt (Nearest-Neighbor in Z)
static void inline
analyzeBestTilt(VolumeValueWindGatherer& vv, LayerValue& layer, AngleDegs& at,
  float& value, bool& isGood, bool& inBeam, bool& terrainBlocked,
  const SentinelDouble& myMissing, const SentinelDouble& myUnavailable)
{
  static const float TERRAIN_PERCENT  = .50;
  static const float BEAMWIDTH_THRESH = .50;

  if ((layer.getTerrainCBBPercent() > TERRAIN_PERCENT) || (layer.getTerrainBeamHitBottom())) {
    terrainBlocked = true;
  }

  // Verify the voxel is actually inside the half-power beamwidth of this tilt
  const double centerBeamDelta = std::abs(at - layer.getElevationDegs());
  inBeam = centerBeamDelta <= BEAMWIDTH_THRESH;

  value  = layer.value;
  isGood = Constants::isGood(value);

  if (!terrainBlocked && inBeam && isGood) {
    // 1. Convert geometric angles to radians
    const float RAD = 0.01745329251f;
    const float elevRad = layer.getElevationDegs() * RAD;
    
    // Virtual Azimuth is retrieved from the pre-cached AzRanElevCache in rFusion1
    const float azRad = vv.virtualAzDegs * RAD; 

    // 2. Calculate Directional Cosines
    // Math Note: In meteorology, azimuth is CW from North. 
    // Therefore X (East) = sin(az), Y (North) = cos(az)
    const float cx = std::cos(elevRad) * std::sin(azRad);
    const float cy = std::cos(elevRad) * std::cos(azRad);

    // 3. Set the payload
    vv.set(value, cx, cy);

  } else {
    // Mark as missing or unavailable to pass cleanly to Stage 2
    if (inBeam) {
      vv.dataValue = myMissing;
    } else {
      vv.dataValue = myUnavailable;
    }
  }
}
}

void
WindResolver::calc(VolumeValue * vvp)
{
  static const float HEIGHT_THRESH_KMS = .5;
  auto& vv = *(VolumeValueWindGatherer *) (vvp);

  // Pull the raw data pointers for the nearest upper and lower tilts
  bool haveLower = queryLower(vv);
  bool haveUpper = queryUpper(vv);

  float dLowerKMs;
  float dUpperKMs;

  // Calculate vertical distance to the tilts
  if (haveLower) {
    dLowerKMs = vv.getAtHeightKMs() - vv.getLowerValue().heightKMs;
    if (dLowerKMs > HEIGHT_THRESH_KMS) { haveLower = false; }
  }
  
  if (haveUpper) {
    dUpperKMs = vv.getUpperValue().heightKMs - vv.getAtHeightKMs();
    if (dUpperKMs > HEIGHT_THRESH_KMS) { haveUpper = false; }
  }

  // STAGE 1 RULE: NO INTERPOLATION FOR VELOCITY.
  // We strictly select the single nearest tilt to avoid the directional shear trap.
  if (haveLower && haveUpper) {
    if (dLowerKMs < dUpperKMs) {
      haveUpper = false; 
    } else {
      haveLower = false; 
    }
  }

  bool terrainBlocked = false;
  bool inBeam         = false;
  bool isGood         = false;
  float value;

  // Process the winning tilt to generate the 5 payload components
  if (haveLower) {
    analyzeBestTilt(vv, vv.getLowerValue(), vv.virtualElevDegs, value, isGood, inBeam, terrainBlocked, myMissing, myUnavailable);
  } else if (haveUpper) {
    analyzeBestTilt(vv, vv.getUpperValue(), vv.virtualElevDegs, value, isGood, inBeam, terrainBlocked, myMissing, myUnavailable);
  } else {
    vv.dataValue = myUnavailable;
  }
}
