#pragma once

#include <rData.h>
#include <rTime.h>
#include <rLLH.h>
#include <rBitset.h>
#include <rRAPIOData.h>
#include <rDataGrid.h>
#include <rBinaryTable.h>
#include <rVolumeValueResolver.h>

#include <vector>

namespace rapio {
class RAPIOFusionOneAlg;

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

  /** Create a stage two data (for receive at moment) */
  Stage2Data(const std::string& radarName,
    const std::string         & typeName,
    const std::string         & units,
    const LLH                 & center,
    size_t                    xBase,
    size_t                    yBase,
    std::vector<size_t>       dims
  ) : myMissingSet(dims, 1),
    myAddMissingCounter(0), myDimensions(dims)
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

  /** Create a stage two data from existing table for reading. */
  Stage2Data(std::shared_ptr<FusionBinaryTable> t,
    std::vector<size_t>                         dims)
    : myMissingSet({ 1, 1 }),
    myAddMissingCounter(0), myDimensions({ 1, 1 })
  {
    myTable = t;
  };

  // -----------------------------------------------------------
  // Stage one adding and sending or 'finalizing' values...

  /** Initialize a volume value IO for sending data.  Typically called by stage1 to
   * prepare to send the grid resolved values */
  virtual bool
  initForSend(
    const std::string   & radarName,
    const std::string   & typeName,
    const std::string   & units,
    const LLH           & center,
    size_t              xBase,
    size_t              yBase,
    std::vector<size_t> dims
  ) override
  {
    // Offload to a binary table, even though we might write netcdf
    // FIXME: The netcdf requires a copy vs raw which doesn't.  We could make two
    // subclasses to optimize for each usage case at some point
    myMissingSet        = Bitset(dims, 1);
    myAddMissingCounter = 0;
    myDimensions        = dims;
    myTable = std::make_shared<FusionBinaryTable>();
    myTable->setString("Sourcename", radarName);
    myTable->setString("Radarname", radarName);
    myTable->setString("Typename", typeName);
    myTable->setUnits(units);
    myTable->setLocation(center);
    myTable->setLong("xBase", xBase);
    myTable->setLong("yBase", yBase);
    return true;
  }

  /** Add data to us for sending to stage2, only used by stage one */
  virtual void
  add(VolumeValue * vvp, short x, short y, short z) override;

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

protected:

  // Meta information for this output
  Bitset myMissingSet;                        ///< Bitfield of missing values gathered during creation
  size_t myAddMissingCounter;                 ///< Number of missing values in bitfield
  std::vector<size_t> myDimensions;           ///< Sizes of the grid in x,y,z (only used for RLE calculation write)
  std::shared_ptr<FusionBinaryTable> myTable; ///< Table used for storage
};
}
