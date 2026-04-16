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

    // 2. Give the subclass a chance to prep its specific outputs
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
    setTerrainData(ourTerrainGridCache[myTerrainFile], ourTerrainProjCache[myTerrainFile]);
    return;
  }

  // Cache miss, load from disk
  fLogInfo("VolumeAlgorithm: Loading terrain grid from {}...", myTerrainFile);
  auto newTerrainGrid = IODataType::read<LatLonGrid>(myTerrainFile);

  if (newTerrainGrid == nullptr) {
    fLogSevere("VolumeAlgorithm: Failed to load terrain grid at {}.", myTerrainFile);
  } else {
    setTerrainData(newTerrainGrid);

    // Store in global cache
    ourTerrainGridCache[myTerrainFile] = myTerrainGrid;
    ourTerrainProjCache[myTerrainFile] = myTerrainProj;
  }
}

void
VolumeAlgorithm::setTerrainData(std::shared_ptr<LatLonGrid> grid,
  std::shared_ptr<LatLonGridProjection>                     proj)
{
  myTerrainGrid = grid;
  if (proj) {
    myTerrainProj = proj;
  } else if (grid) {
    myTerrainProj = std::make_shared<LatLonGridProjection>(Constants::PrimaryDataName, grid.get());
  } else {
    myTerrainProj = nullptr;
  }
}

void
VolumeAlgorithm::iterate(std::shared_ptr<LatLonHeightGrid> input, LatLonHeightGridCallback& callback, IterateMode mode)
{
  LatLonHeightGridIterator iter(*input);

  // Might have a remapped cache at some point, currently we're projecting which
  // saves ram but increases CPU.  Part of why we hide this inside VolumeAlgorithm
  if (myTerrainProj) {
    iter.setTerrainProj(myTerrainProj);
  }

  // When NSE is added, it goes right here:
  // if (myH263Proj) iter.setH263(myH263Proj);
  // if (myH233Proj) iter.setH233(myH233Proj);

  // Route to the explicit iterator methods
  switch (mode) {
      case IterateMode::Voxels:
        iter.iterateVoxels(callback);
        break;
      case IterateMode::ColumnsUp:
        iter.iterateUpColumns(callback);
        break;
      case IterateMode::ColumnsDown:
        iter.iterateDownColumns(callback);
        break;
  }
}
