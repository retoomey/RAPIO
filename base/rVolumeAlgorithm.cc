#include "rVolumeAlgorithm.h"
#include "rLatLonHeightGrid.h"
#include "rIODataType.h"

using namespace rapio;

// Initialize static cache members
std::map<std::string, std::shared_ptr<LatLonGrid> > VolumeAlgorithm::ourTerrainGridCache;
std::map<std::string, std::shared_ptr<LatLonGridProjection> > VolumeAlgorithm::ourTerrainProjCache;
std::mutex VolumeAlgorithm::ourTerrainMutex;

void
VolumeAlgorithm::declareOptions(RAPIOOptions& o)
{
  o.optional("terrain", "", "Path to a NetCDF terrain file (optional for most, required for some)");
}

void
VolumeAlgorithm::processOptions(RAPIOOptions& o)
{
  myTerrainFile = o.getString("terrain");
}

void
VolumeAlgorithm::setupVolumeProcessing(std::shared_ptr<LatLonHeightGrid> llg)
{
  // Do all the caching work, etc. we do for the incoming data
  if (llg != nullptr) {
    // 1. Prepare shared resources (Terrain)
    loadTerrainGrid();

    // 2. Pre-calculate height level arrays for subclasses
    const size_t numHts = llg->getNumLayers();
    myHLevels.resize(numHts);
    myHLevelsKMs.resize(numHts);
    for (size_t h = 0; h < numHts; h++) {
      myHLevels[h]    = llg->getLayerValue(h);
      myHLevelsKMs[h] = myHLevels[h] / 1000.0f;
    }

    // 3. Give the subclass a chance to prep its specific outputs
    checkOutputGrids(llg);
  }
}

void
VolumeAlgorithm::processNewData(RAPIOData& d)
{
  auto llg = d.datatype<LatLonHeightGrid>();

  if (llg != nullptr) {
    setupVolumeProcessing(llg);
    processVolume(llg, this);
  }
}

bool
VolumeAlgorithm::checkCoverageChange(std::shared_ptr<LatLonHeightGrid> input)
{
  const LLCoverageArea coverage = input->getLLCoverageArea();

  if (myCachedCoverage != coverage) {
    myCachedCoverage = coverage;
    return true;
  }
  return false;
}

void
VolumeAlgorithm::loadTerrainGrid()
{
  if (myTerrainFile.empty() || (myTerrainGrid != nullptr)) {
    return; // No file specified, or already loaded by this instance
  }

  std::lock_guard<std::mutex> lock(ourTerrainMutex);

  // Check if another VolumeAlgorithm already loaded this specific terrain file
  if (ourTerrainGridCache.count(myTerrainFile) > 0) {
    fLogDebug("VolumeAlgorithm: Using cached terrain grid for {}.", myTerrainFile);
    myTerrainGrid = ourTerrainGridCache[myTerrainFile];
    myTerrainProj = ourTerrainProjCache[myTerrainFile];
    return;
  }

  // Cache miss, load from disk
  fLogInfo("VolumeAlgorithm: Loading terrain grid from {}...", myTerrainFile);
  myTerrainGrid = IODataType::read<LatLonGrid>(myTerrainFile);

  if (myTerrainGrid == nullptr) {
    fLogSevere("VolumeAlgorithm: Failed to load terrain grid at {}.", myTerrainFile);
  } else {
    myTerrainProj = std::make_shared<LatLonGridProjection>(Constants::PrimaryDataName, myTerrainGrid.get());

    // Store in global cache
    ourTerrainGridCache[myTerrainFile] = myTerrainGrid;
    ourTerrainProjCache[myTerrainFile] = myTerrainProj;
  }
}
