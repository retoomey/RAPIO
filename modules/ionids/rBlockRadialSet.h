#pragma once

#include <rIONIDS.h>
#include <rBinaryIO.h>

namespace rapio {
/** NIDS block containing a RadialSet */
class BlockRadialSet : public NIDSBlock {
public:

  /** Internal NIDS storage of a radial.
   * Keeping this deliberately simple for the moment,
   * not 100% the most efficient storage yet. */
  class RadialData : public Data {
public:
    float start_angle;
    float delta_angle;
    bool inShorts;
    std::vector<char> data;
  };

  /** Read ourselves from buffer */
  virtual void
  read(StreamBuffer& s) override;

  /** Write ourselves to buffer */
  virtual void
  write(StreamBuffer& s) override;

  /** Debug dump */
  void
  dump();

  // Field access ----------------------------------

  /** Get packet code */
  float
  getPacketCode() const { return myPacketCode; };

  /** Gets index of first range bin */
  short
  getFirstRangeBinIndex() const { return myIndexFirstBin; };

  /** Gets number of range bin */
  short
  getNumOfRangeBin() const { return myNumRangeBin; };

  /** Gets I center of sweep */
  short
  getICenterOfSweep() const { return myCenterOfSweepI; };

  /** Gets J center of sweep */
  short
  getJCenterOfSweep() const { return myCenterOfSweepJ; };

  /** Get scale factor */
  float
  getScaleFactor() const { return myScaleFactor * 0.001; };

  /** Get the number of radials */
  size_t getNumRadials(){ return myNumRadials; }

  // End Field access ----------------------------------

  std::vector<std::vector<char> > myRawData;

  /** Store raw radial information */
  std::vector<RadialData> myRadials;

protected:

  short myPacketCode;     ///< Packet Type x'AF1F' or 16 or 28
  short myIndexFirstBin;  ///< Index of first range bin
  short myNumRangeBin;    ///< Number of range bins comprising a radial
  short myCenterOfSweepI; ///< I coordinate of center of sweep
  short myCenterOfSweepJ; ///< J coordinate of center of sweep
  short myScaleFactor;    ///< Number of pixels per range bin
  short myNumRadials;     ///< Total number of radials in products
};
}
