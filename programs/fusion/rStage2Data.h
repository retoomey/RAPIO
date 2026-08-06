#pragma once

#include <rTime.h>
#include <rLLH.h>
#include <rBitset.h>
#include <rRAPIOData.h>
#include <rDataGrid.h>
#include <rFusionBinaryTable.h>
#include <rVolumeValueResolver.h>
#include <rPartitionInfo.h>

#include <vector>

namespace rapio {
class RAPIOFusionOneAlg;

/** Store a single tile or area. Depending on our partitioning mode, we may have one or N of these. */
class Stage2Storage {
protected:
  std::vector<size_t> myDimensions;           ///< Sizes of the grid in x,y,z (only used for RLE calculation write)
  Bitset1 myMissingSet;                       ///< Bitfield of missing values gathered during creation
  std::shared_ptr<FusionBinaryTable> myTable; ///< Table used for storage of actual values
  std::string mySubFolder;                    
  size_t myMissingRLECounter; ///< After a send, the count of sent missing (RLE compressed)
  size_t myAddValueCounter;   ///< Number of valid values sent to add
  size_t myAddMissingCounter; ///< Number of missing values sent to add
public:

  /** Create a storage for this */
  Stage2Storage(
    const std::string   & radarName,
    const std::string   & typeName,
    const std::string   & units,
    const LLH           & center,
    const LLCoverageArea& radarGrid,
    const bool          noMissingSet) :
    myDimensions({ radarGrid.getNumX(), radarGrid.getNumY(), radarGrid.getNumZ() }),
    myMissingSet(myDimensions),
    myMissingRLECounter(0), myAddValueCounter(0), myAddMissingCounter(0)
  {
    myTable = std::make_shared<FusionBinaryTable>();
    if (noMissingSet) {
      myTable->setUseMissingAsUnavailable();
    }
    myTable->setString("Sourcename", radarName);
    myTable->setString("Radarname", radarName);
    myTable->setString("Typename", typeName);
    myTable->setUnits(units);
    myTable->setLocation(center);
    myTable->setLong("xBase", radarGrid.getStartX());
    myTable->setLong("yBase", radarGrid.getStartY());
  }

  /** Get the number of missing added to us (uncompressed) */
  size_t getAddedMissingCount() { return myAddMissingCounter; }

  /** Get the number of value values added to us (uncompressed) */
  size_t getAddedValueCount() { return myAddValueCounter; }

  /** After a send, get the number of missing values (compressed) */
  size_t getSentMissingRLECount() { return myMissingRLECounter; }

  /** Set the subfolder to write to */
  void setSubFolder(const std::string& folder) { mySubFolder = folder; }

  /** Add a value to our storage */
  inline void add(VolumeValue * vvp, short x, short y, short z)
  {
    // Our particular resolver VolumeValue
    auto& vv = *static_cast<VolumeValueWeightAverage *>(vvp);

    if (vv.topSum == Constants::MissingData) {
      // Only store x,y,z for a missing and we'll do a RLE compress of x,y,z locations
      myMissingSet.set13D(x, y, z);
      myAddMissingCounter++;
      return;
    }

    if (vv.topSum != Constants::DataUnavailable) {
      myTable->add(vv.topSum, vv.bottomSum, x, y, z);
      myAddValueCounter++;
    }
  }

  /** Send/write our tile. */
  void send(RAPIOAlgorithm * alg, Time aTime, const std::string& asName);

  /** Number of true non-missing values expected */
  size_t getValueCount() { return myTable->myNums.size(); }

  /** Number of missing values expected */
  size_t getMissingCount() { return myTable->myXMissings.size(); }

  /** Get data from us, only used by stage two.
   * Note this streams out until returning false */
  inline bool get(float& n, float& d, short& x, short& y, short& z) { return myTable->get(n, d, x, y, z); }

  /** Get the radarname */
  std::string getRadarName() { std::string r; myTable->getString("Radarname", r); return r; }

  /** Get the typename */
  std::string getTypeName() { std::string t; myTable->getString("Typename", t); return t; }

  /** Get the units */
  std::string getUnits() { return myTable->getUnits(); }

  /** Get sent time, or epoch */
  Time getTime() { return myTable->getTime(); }

  /** Set sent time */
  void setTime(const Time& t) { myTable->setTime(t); }

  /** Get the location */
  LLH getLocation() { return myTable->getLocation(); }

  /** Get the XBase value or starting Lon/X cell of the grid we represent */
  size_t getXBase() { long xBase = 0; myTable->getLong("xBase", xBase); return xBase; }

  /** Get the YBase value or starting Lat/Y cell of the grid we represent */
  size_t getYBase() { long yBase = 0; myTable->getLong("yBase", yBase); return yBase; }

  /** Compress bit array in RLE.
   *  This reduces size quite a bit since weather tends to clump up.
   */
  void RLE();
};

/** Handles stage 1 to stage 2 writing */
class Stage2Data : public VolumeValueIO {
public:

  Stage2Data() {}

  virtual bool initForSend(
    const std::string & radarName,
    const std::string & typeName,
    const std::string & units,
    const LLH         & center,
    const bool        noMissingSet,
    const PartitionInfo & partition,
    const LLCoverageArea& radarGrid) override
  {
    for (size_t i = 0; i < partition.size(); ++i) {
      std::shared_ptr<Stage2Storage> newOne =
        std::make_shared<Stage2Storage>(radarName, typeName, units, center, radarGrid, noMissingSet);

      if ((partition.getPartitionType() == PartitionInfo::Type::tile)) {
        std::stringstream s;
        s << "partition" << (i + 1);
        newOne->setSubFolder(s.str());
      }
      myStorage.push_back(newOne);
    }
    return true;
  }

  virtual void add(VolumeValue * vvp, short x, short y, short z, size_t partIndex) override
  {
    if (partIndex >= myStorage.size()) {
      fLogSevere("Index is {} and storage is {}", partIndex, myStorage.size());
      exit(1);
    }
    myStorage[partIndex]->add(vvp, x, y, z);
  }

  virtual void send(RAPIOAlgorithm * alg, Time aTime, const std::string& asName) override;

protected:
  std::vector<std::shared_ptr<Stage2Storage> > myStorage;
};
}
