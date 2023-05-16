#pragma once

#include <rData.h>
#include <rTime.h>
#include <rLLH.h>

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
    const LLH                 & center
  ) : myRadarName(radarName), myTypeName(typeName), myUnits(units), myCenter(center)
  { };

  /** Add data to us for sending to stage2 */
  void
  add(float v, float w, short x, short y, short z)
  {
    stage2Values.push_back(v);
    stage2Weights.push_back(w);
    stage2Xs.push_back(x);
    stage2Ys.push_back(y);
    stage2Zs.push_back(z); // We're hiding that z is a char
  }

  /** Send/write stage2 data.  Give an algorithm pointer so we call do alg things if needed. */
  void
  send(RAPIOFusionOneAlg * alg, Time aTime, const std::string& asName);

protected:
  // Meta information for this output
  std::string myRadarName;
  std::string myTypeName;
  std::string myUnits;
  LLH myCenter;

  // Stored data to process
  std::vector<float> stage2Values;
  std::vector<float> stage2Weights;
  std::vector<short> stage2Xs; // Optimization: Possible byte could be enough with extra attribute info
  std::vector<short> stage2Ys;
  std::vector<char> stage2Zs; // z assumed small
};
}
