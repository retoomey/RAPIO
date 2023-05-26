#pragma once

#include <rData.h>
#include <rTime.h>
#include <rLLH.h>
#include <rBitset.h>
#include <rRAPIOData.h>

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
    const std::string         & units,
    const LLH                 & center,
    std::vector<size_t>       dims
  ) : myRadarName(radarName), myTypeName(typeName), myUnits(units), myCenter(center), myMissingSet(dims, 1),
    myDimensions(dims)
  { };

  /** Add data to us for sending to stage2 */
  void
  add(float v, float w, short x, short y, short z);

  /** RLE missing values, reduces size quite a bit since weather tends to clump up. */
  void
  RLE();

  /** Send/write stage2 data.  Give an algorithm pointer so we call do alg things if needed. */
  void
  send(RAPIOFusionOneAlg * alg, Time aTime, const std::string& asName);

  /** Receive stage2 data.  Used by stage2 to read things back in */
  static void
  receive(RAPIOData& d);

protected:
  // Meta information for this output
  std::string myRadarName;
  std::string myTypeName;
  std::string myUnits;
  LLH myCenter;
  BitsetDims myMissingSet;
  std::vector<size_t> myDimensions;

  // Stored data to process
  std::vector<float> myValues;
  std::vector<float> myWeights;
  std::vector<short> myXs; // Optimization: Possible byte could be enough with extra attribute info
  std::vector<short> myYs;
  std::vector<char> myZs; // z assumed small

  // We should never weight against a missing value...so store all of them separate to save space.
  // Since we're sparse we can just store the x,y,z RLE
  std::vector<short> myXMissings;
  std::vector<short> myYMissings;
  std::vector<char> myZMissings;
  std::vector<short> myLMissings;
};
}
