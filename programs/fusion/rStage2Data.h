#pragma once

#include <rData.h>
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

/** Store a single tile or area.  Depending on our partitioning mode, we may have one or N of these. */
class Stage2Storage : public Data {
protected:
  std::vector<size_t> myDimensions;           ///< Sizes of the grid in x,y,z (only used for RLE calculation write)
  Bitset1 myMissingSet;                       ///< Bitfield of missing values gathered during creation
  std::shared_ptr<FusionBinaryTable> myTable; ///< Table used for storage of actual values
  std::string mySubFolder;                    ///< If not empty, subfolder of -o such as 'partition1'
  // Adding/Sending stat collection
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
  size_t
  getAddedMissingCount()
  {
    return myAddMissingCounter;
  }

  /** Get the number of value values added to us (uncompressed) */
  size_t
  getAddedValueCount()
  {
    return myAddValueCounter;
  }

  /** After a send, get the number of missing values (compressed) */
  size_t
  getSentMissingRLECount()
  {
    return myMissingRLECounter;
  }

  /** Set the subfolder to write to */
  void
  setSubFolder(const std::string& folder)
  {
    mySubFolder = folder;
  }

  /** Add a value to our storage */
  inline void
  add(VolumeValue * vvp, short x, short y, short z)
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
  void
  send(RAPIOAlgorithm * alg, Time aTime, const std::string& asName);

  /** Number of true non-missing values expected */
  size_t
  getValueCount()
  {
    return myTable->myNums.size();
  }

  /** Number of missing values expected */
  size_t
  getMissingCount()
  {
    return myTable->myXMissings.size();
  }

  /** Get data from us, only used by stage two.
   * Note this streams out until returning false */
  inline bool
  get(float& n, float& d, short& x, short& y, short& z)
  {
    return myTable->get(n, d, x, y, z);
  }

  /** Get the radarname */
  std::string
  getRadarName()
  {
    std::string r;

    myTable->getString("Radarname", r);
    return r;
  }

  /** Get the typename */
  std::string
  getTypeName()
  {
    std::string t;

    myTable->getString("Typename", t);
    return t;
  }

  /** Get the units */
  std::string
  getUnits()
  {
    return myTable->getUnits();
  }

  /** Get sent time, or epoch */
  Time
  getTime()
  {
    return myTable->getTime();
  }

  /** Set sent time */
  void
  setTime(const Time& t)
  {
    myTable->setTime(t);
  }

  /** Get the location */
  LLH
  getLocation()
  {
    return myTable->getLocation();
  }

  /** Get the XBase value or starting Lon/X cell of the grid we represent */
  size_t
  getXBase()
  {
    long xBase = 0;

    myTable->getLong("xBase", xBase);
    return xBase;
  }

  /** Get the YBase value or starting Lat/Y cell of the grid we represent */
  size_t
  getYBase()
  {
    long yBase = 0;

    myTable->getLong("yBase", yBase);
    return yBase;
  }

  /** Compress bit array in RLE.
   *  This reduces size quite a bit since weather tends to clump up.
   */
  void
  RLE();
};

/**  Handle dealing with the stage 1 to stage 2 data files
 * or whatever other techniques we try.  This could be raw
 * files similar to w2merger or a different file or even
 * web sockets or whatever ends up working best.
 *
 * Mostly this wraps our FusionBinaryTable class, but I'm leaving this
 * here since to hide the internals of how stage1 to stage2 data is handled.
 *
 * @author Robert Toomey
 */
class Stage2Data : public VolumeValueIO {
public:

  /** Empty creation for stl */
  Stage2Data(){ }

  /** Create a stage two data (for receive by netcdf) */
  Stage2Data(const std::string& radarName,
    const std::string         & typeName,
    const std::string         & units,
    const LLH                 & center,
    size_t                    xBase,
    size_t                    yBase,
    std::vector<size_t>       dims
  ) : myMissingSet(dims), myDimensions(dims)
  {
    // Offload to a binary table, even though we might write netcdf
    // FIXME: The netcdf requires a copy vs raw which doesn't.  We could make two
    // subclasses to optimize for each usage case at some point
    myTable = std::make_shared<FusionBinaryTable>();
    myTable->setString("Sourcename", radarName);
    myTable->setString("Radarname", radarName);
    myTable->setString("Typename", typeName);
    myTable->setUnits(units);
    myTable->setLocation(center);
    myTable->setLong("xBase", xBase);
    myTable->setLong("yBase", yBase);
  };

  /** Create a stage two data from existing table for receive by raw file. */
  Stage2Data(std::shared_ptr<FusionBinaryTable> t,
    std::vector<size_t>                         dims)
    : myMissingSet({ 1, 1 }), myDimensions({ 1, 1 })
  {
    myTable = t;
  };

  // -----------------------------------------------------------
  // Stage one adding and sending or 'finalizing' values...

  /** Initialize a volume value IO for sending data.  Typically called by stage1 to
   * prepare to send the grid resolved values */
  virtual bool
  initForSend(
    const std::string & radarName,
    const std::string & typeName,
    const std::string & units,
    const LLH         & center,
    const bool        noMissingSet,

    // Partitioning info
    const PartitionInfo & partition,
    // Grid based stuff.  For tiling we'll need all the info
    const LLCoverageArea& radarGrid
  ) override
  {
    // For 'one' partition, it should be == to the global CONUS grid.
    // This is for 'none' --> 1 global partition
    // This is for 'tile' --> N smaller partitions
    // FIXME: for 'tree' I think just the output folder will change, we won't
    // need N tables for that...
    for (size_t i = 0; i < partition.size(); ++i) {
      // It's interesting that we pass the local radar grid to the storage, since the missing bit
      // array there uses the radar grid (x,y,z) not the partition coordinates.
      // std::make_shared<Stage2Storage>(radarName, typeName, units, center, partition.myPartitions[i]);
      std::shared_ptr<Stage2Storage> newOne =
        std::make_shared<Stage2Storage>(radarName, typeName, units, center, radarGrid, noMissingSet);

      // Set the partition subfolder.
      if ((partition.myParamType == PartitionType::tile)) {
        std::stringstream s;
        s << "partition" << (i + 1);
        newOne->setSubFolder(s.str());
      }

      myStorage.push_back(newOne);
    }

    return true;
  } // initForSend

  /** Add data to us for sending to stage2, only used by stage one */
  virtual void
  add(VolumeValue * vvp, short x, short y, short z, size_t partIndex) override
  {
    if (partIndex >= myStorage.size()) {
      LogSevere("Index is " << partIndex << " and storage is " << myStorage.size() << "\n");
      exit(1);
    }
    myStorage[partIndex]->add(vvp, x, y, z);
  }

  /** Send/write stage2 data.  Give an algorithm pointer so we call do alg things if needed. */
  virtual void
  send(RAPIOAlgorithm * alg, Time aTime, const std::string& asName) override;

  // -----------------------------------------------------------
  // Stage two receiving and getting values...

  /** Receive stage2 data.  Used by stage2 to read things back in */
  static std::shared_ptr<Stage2Data>
  receive(RAPIOData& d);

  /** Number of true non-missing values expected */
  size_t
  getValueCount()
  {
    return myTable->myNums.size();
  }

  /** Number of missing values expected */
  size_t
  getMissingCount()
  {
    return myTable->myXMissings.size();
  }

  /** Get data from us, only used by stage two.
   * Note this streams out until returning false */
  inline bool
  get(float& n, float& d, short& x, short& y, short& z)
  {
    return myTable->get(n, d, x, y, z);
  }

  /** Get the missing mode from this source's data */
  bool
  getNoMissingSet()
  {
    return myTable->getUseMissingAsUnavailable();
  }

  /** Get the radarname */
  std::string
  getRadarName()
  {
    std::string r;

    myTable->getString("Radarname", r);
    return r;
  }

  /** Get the typename */
  std::string
  getTypeName()
  {
    std::string t;

    myTable->getString("Typename", t);
    return t;
  }

  /** Get the units */
  std::string
  getUnits()
  {
    return myTable->getUnits();
  }

  /** Get sent time, or epoch */
  Time
  getTime()
  {
    return myTable->getTime();
  }

  /** Set sent time */
  void
  setTime(const Time& t)
  {
    myTable->setTime(t);
  }

  /** Get the location */
  LLH
  getLocation()
  {
    return myTable->getLocation();
  }

  /** Get the XBase value or starting Lon/X cell of the grid we represent */
  size_t
  getXBase()
  {
    long xBase = 0;

    myTable->getLong("xBase", xBase);
    return xBase;
  }

  /** Get the YBase value or starting Lat/Y cell of the grid we represent */
  size_t
  getYBase()
  {
    long yBase = 0;

    myTable->getLong("yBase", yBase);
    return yBase;
  }

protected:

  // --------------------------------------------
  // For receive currently just store a single collection of data:
  // Meta information for this output
  // FIXME: This will probably go away, read can also use a Stage2Storage
  // But for now read only reads one table so we'll keep this separate
  Bitset1 myMissingSet;                       ///< Bitfield of missing values gathered during creation
  std::vector<size_t> myDimensions;           ///< Sizes of the grid in x,y,z (only used for RLE calculation write)
  std::shared_ptr<FusionBinaryTable> myTable; ///< Table used for storage

  // --------------------------------------------
  // For send need N storage
  std::vector<std::shared_ptr<Stage2Storage> > myStorage; ///< Storage for a tile or area
};
}
