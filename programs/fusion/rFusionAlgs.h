#pragma once

#include <rThreadGroup.h>
#include <rVolumeAlgorithm.h>
#include <vector>
#include <memory>

namespace rapio {
// 1. The Composite Callback (Loop Fusion)
class MultiHeightGridCallback : public LatLonHeightGridCallback {
public:
  void
  addCallback(LatLonHeightGridCallback * cb)
  {
    if (cb) { myCallbacks.push_back(cb); }
  }

  bool
  empty() const { return myCallbacks.empty(); }

  virtual void
  handleBeginLoop(LatLonHeightGridIterator * it, const LatLonHeightGrid& grid) override
  {
    for (auto cb : myCallbacks) { cb->handleBeginLoop(it, grid); }
  }

  virtual void
  handleBeginColumn(LatLonHeightGridIterator * it) override
  {
    for (auto cb : myCallbacks) { cb->handleBeginColumn(it); }
  }

  virtual void
  handleVoxel(LatLonHeightGridIterator * it) override
  {
    // The Hot Loop: One memory fetch, N algorithms process it
    for (auto cb : myCallbacks) { cb->handleVoxel(it); }
  }

  virtual void
  handleEndColumn(LatLonHeightGridIterator * it) override
  {
    for (auto cb : myCallbacks) { cb->handleEndColumn(it); }
  }

  virtual void
  handleEndLoop(LatLonHeightGridIterator * it, const LatLonHeightGrid& grid) override
  {
    for (auto cb : myCallbacks) { cb->handleEndLoop(it, grid); }
  }

private:
  std::vector<LatLonHeightGridCallback *> myCallbacks;
};

// 2. The Spatial Thread Task (Thread the Grid)
class GridChunkTask : public ThreadTask {
public:
  GridChunkTask(std::shared_ptr<LatLonHeightGrid> grid,
    std::shared_ptr<MultiHeightGridCallback> multiCb,
    size_t startY, size_t endY)
    : myGrid(grid), myMultiCb(multiCb), myStartY(startY), myEndY(endY){ }

  virtual void
  execute() override
  {
    // Note: You must update LatLonHeightGridIterator to accept startY/endY bounds!
    LatLonHeightGridIterator iter(*myGrid, myStartY, myEndY);

    // We force ColumnsDown so VIL and MaxAGL can share the loop
    iter.iterateDownColumns(*myMultiCb);

    markDone();
  }

private:
  std::shared_ptr<LatLonHeightGrid> myGrid;
  std::shared_ptr<MultiHeightGridCallback> myMultiCb;
  size_t myStartY;
  size_t myEndY;
};

// 3. The Orchestrator
class RAPIOFusionAlgs : public rapio::RAPIOAlgorithm {
public:
  RAPIOFusionAlgs(){ };
  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;
  virtual void
  processOptions(rapio::RAPIOOptions& o) override;
  virtual void
  processNewData(rapio::RAPIOData& d) override;
  virtual void
  processHeartbeat(const Time& n, const Time& p) override;

  void
  firstDataSetup();

protected:

  /** Algorithm module/programs loaded dynamically */
  std::vector<std::shared_ptr<VolumeAlgorithm> > myLoadedAlgorithms;
};
} // namespace rapio
