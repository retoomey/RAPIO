#pragma once

#include "rVolumeValueResolver.h"
#include "rRAPIOAlgorithm.h"
#include "rBinaryTable.h"
#include "rBitset.h"
#include "rPartitionInfo.h"
#include "rLLCoverageArea.h"
#include "rWindBinaryTable.h"

namespace rapio {

// ----------------------------------------------------------------------------
// 1. The custom VolumeValue to hold our 5 components + vr
// ----------------------------------------------------------------------------
class VolumeValueWindGatherer : public VolumeValue
{
public:
  inline void
  set(float vr_, float cx_, float cy_)
  {
    vr = vr_;
    cx = cx_;
    cy = cy_;
    
    // Calculate the components needed for the Normal Equations
    M11 = cx * cx;
    M22 = cy * cy;
    M12 = cx * cy;
    P1  = cx * vr;
    P2  = cy * vr;

    // We store the raw velocity in dataValue so the legacy 
    // 2D CAPPIL generator still has something to plot for debugging
    dataValue = vr; 
  }

  float vr;
  float cx;
  float cy;

  // The 5 payload components
  float M11;
  float M22;
  float M12;
  float P1;
  float P2;
};

// ----------------------------------------------------------------------------
// 3. Stage 2 Storage Wrapper (Handles RLE Compression per partition)
// ----------------------------------------------------------------------------
class WindStage2Storage {
public:
  WindStage2Storage(
    const std::string   & radarName,
    const std::string   & typeName,
    const std::string   & units,
    const LLH           & center,
    const LLCoverageArea& radarGrid,
    const bool          noMissingSet);

  void setSubFolder(const std::string& folder) { mySubFolder = folder; }

  inline void
  add(VolumeValue * vvp, short x, short y, short z)
  {
    auto& vv = *(VolumeValueWindGatherer *) (vvp);
    
    // If the data is missing or out of bounds, we flag it in the Bitset for RLE later
    if (vv.dataValue == Constants::MissingData) {
      myMissingSet.set13D(x, y, z);
      myAddMissingCounter++;
      return;
    }
    
    // If it's valid, we add the 5 components to the table
    if (vv.dataValue != Constants::DataUnavailable) {
      myTable->add(vv.M11, vv.M22, vv.M12, vv.P1, vv.P2, x, y, z);
      myAddValueCounter++;
    }
  }

  void send(RAPIOAlgorithm * alg, Time aTime, const std::string& asName);
  void RLE();

protected:
  std::vector<size_t> myDimensions;
  Bitset1 myMissingSet;
  std::shared_ptr<WindBinaryTable> myTable;
  std::string mySubFolder;
  
  size_t myAddValueCounter;
  size_t myAddMissingCounter;
};

// ----------------------------------------------------------------------------
// 4. The VolumeValueIO (Routes data to the correct partition storage)
// ----------------------------------------------------------------------------
class WindVolumeValueIO : public VolumeValueIO {
public:
  WindVolumeValueIO() { }

  virtual bool
  initForSend(
    const std::string    & radarName,
    const std::string    & typeName,
    const std::string    & units,
    const LLH            & center,
    const bool           noMissingSet,
    const PartitionInfo  & partition,
    const LLCoverageArea & radarGrid) override;

  virtual void
  add(VolumeValue * vvp, short x, short y, short z, size_t partIndex) override;

  virtual void
  send(RAPIOAlgorithm * alg, Time aTime, const std::string& asName) override;

private:
  std::vector<std::shared_ptr<WindStage2Storage>> myStorage;
};

// ----------------------------------------------------------------------------
// 5. The Main Wind Resolver
// ----------------------------------------------------------------------------
class WindResolver : public VolumeValueResolver
{
public:
  WindResolver() {}

  static void introduceSelf();

  virtual std::string getHelpString(const std::string& fkey) override;

  virtual std::shared_ptr<VolumeValueResolver>
  create(const std::string & params) override;

  virtual std::shared_ptr<VolumeValue>
  getVolumeValue() override
  {
    return std::make_shared<VolumeValueWindGatherer>();
  }

  virtual std::shared_ptr<VolumeValueIO>
  getVolumeValueIO() override
  {
    return std::make_shared<WindVolumeValueIO>();
  }

  virtual void calc(VolumeValue * vvp) override;
};

} // namespace rapio
