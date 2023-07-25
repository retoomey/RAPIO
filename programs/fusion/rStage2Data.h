#pragma once

#include <rData.h>
#include <rTime.h>
#include <rLLH.h>
#include <rBitset.h>
#include <rRAPIOData.h>
#include <rDataGrid.h>

#include <vector>

namespace rapio {
class RAPIOFusionOneAlg;

/**  Handle dealing with the stage 1 to stage 2 data files
 * or whatever other techniques we try.  This could be raw
 * files similar to w2merger or a different file or even
 * web sockets or whatever ends up working best.
 *
 * @author Robert Toomey
 */
class Stage2Data : public Data {
public:
  /** Create a stage two data */
  Stage2Data(const std::string& radarName,
    const std::string         & typeName,
    float                     elevDegs,
    const std::string         & units,
    const LLH                 & center,
    size_t                    xBase,
    size_t                    yBase,
    std::vector<size_t>       dims
  ) : myRadarName(radarName), myTypeName(typeName), myElevationDegs(elevDegs), myUnits(units), myCenter(center),
    myXBase(xBase), myYBase(yBase),
    myMissingSet(dims, 1),
    myAddMissingCounter(0), myDimensions(dims), myCounter(0), myMCounter(0), myRLECounter(0)
  { };

  // -----------------------------------------------------------
  // Stage one adding and sending or 'finalizing' values...

  /** Add data to us for sending to stage2, only used by stage one */
  void
  add(float v, float w, short x, short y, short z);

  /** Send/write stage2 data.  Give an algorithm pointer so we call do alg things if needed. */
  void
  send(RAPIOFusionOneAlg * alg, Time aTime, const std::string& asName);

  // -----------------------------------------------------------
  // Stage two receiving and getting values...

  /** Receive stage2 data.  Used by stage2 to read things back in */
  static std::shared_ptr<Stage2Data>
  receive(RAPIOData& d);

  /** Get data from us, only used by stage two.
   * Note this streams out until returning false */
  bool
  get(float& v, float& w, short& x, short& y, short& z);

  /** Get the radarname */
  std::string
  getRadarName()
  {
    return myRadarName;
  }

  /** Get the typename */
  std::string
  getTypeName()
  {
    return myTypeName;
  }

  /** Get the elevation degrees */
  float
  getElevationDegs()
  {
    return myElevationDegs;
  }

  /** Get the units */
  std::string
  getUnits()
  {
    return myUnits;
  }

  /** Get sent time, or epoch */
  Time
  getTime()
  {
    return myTime;
  }

  /** Set sent time */
  void
  setTime(const Time& t)
  {
    myTime = t;
  }

  /** Get the location */
  LLH
  getLocation()
  {
    return myCenter;
  }

  /** Get the XBase value or starting Lon/X cell of the grid we represent */
  size_t
  getXBase()
  {
    return myXBase;
  }

  /** Get the YBase value or starting Lat/Y cell of the grid we represent */
  size_t
  getYBase()
  {
    return myYBase;
  }

  /** Compress bit array in RLE.
   *  This reduces size quite a bit since weather tends to clump up.
   */
  void
  RLE();

protected:

  // Meta information for this output
  std::string myRadarName; ///< Radar name such as KTLX
  std::string myTypeName;  ///< Type name such as Reflectivity
  float myElevationDegs;   ///< Elevation angle in degrees for this data
  std::string myUnits;     ///< Units such as dBZ
  Time myTime;             ///< Global time for this
  LLH myCenter;            ///< Location center of radar
  size_t myXBase;          ///< Base offset of X in the global grid
  size_t myYBase;          ///< Base offset of Y in the global grid

  Bitset myMissingSet;              ///< Bitfield of missing values gathered during creation
  size_t myAddMissingCounter;       ///< Number of missing values in bitfield
  std::vector<size_t> myDimensions; ///< Sizes of the grid in x,y,z

  // Stored data to process
  std::vector<float> myValues;  ///< True data values, not missing
  std::vector<float> myWeights; ///< Weights for true data values
  std::vector<short> myXs;      ///< X location for a true value
  std::vector<short> myYs;      ///< Y location for a true value
  std::vector<char> myZs;       ///< Z value, height for a true value

  // We should never weight against a missing value...so store all of them separate to save space.
  // Since we're sparse we can just store the x,y,z RLE
  std::vector<short> myXMissings; ///< X start for RLE missing range
  std::vector<short> myYMissings; ///< Y start for RLE missing range
  std::vector<char> myZMissings;  ///< Z start for RLE missing range
  std::vector<short> myLMissings; ///< The length of the run in X horizontal

  // Getting back data counters...
  size_t myCounter;    ///< get() stream true value counter iterator
  size_t myMCounter;   ///< get() stream RLE missing counter iterator
  size_t myRLECounter; ///< get() stream RLE length within missing counter iterator
};
}
