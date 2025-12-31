#pragma once

#include <rIONIDS.h>
#include <rBinaryIO.h>

#include <array>

namespace rapio {
/** Product Description Block
 * @author Jeff Brogden, Lulin Song, Robert Toomey
 */
class BlockProductDesc : public NIDSBlock {
public:

  /** Read ourselves from buffer */
  virtual void
  read(StreamBuffer& s) override;

  /** Write ourselves to buffer */
  virtual void
  write(StreamBuffer& s) override;

  /** Debug dump */
  void
  dump();

  /** Get location of product, typically center of data */
  LLH
  getLocation() const { return myLocation; }

  /** Set location of product, typically center of data */
  void setLocation(const LLH& l){ myLocation = l; }

  /** Get product code */
  short
  getProductCode() const { return myProductCode; }

  /** Set product code */
  void setProductCode(short c){ myProductCode = c; }

  /** Get operation mode */
  short
  getOpMode() const { return myOpMode; }

  /** Set operation mode */
  void setOpMode(short m){ myOpMode = m; }

  /** Get vcp if available.  Curious what this is for grids. */
  short
  getVCP() const { return myVCP; }

  /** Set vcp. */
  void setVCP(short v){ myVCP = v; }

  /** Get sequence number */
  short
  getSeqNum() const { return mySeqNumber; }

  /** Set sequence number */
  void setSeqNum(short s){ mySeqNumber = s; }

  /** Get volume scan number */
  short
  getVolScanNum() const { return myVolScanNum; }

  /** Set volume scan number */
  void setVolScanNum(short s){ myVolScanNum = s; }

  /** Get start time of product */
  Time
  getVolumeStartTime() const { return myVolStartTime; }

  /** Set start time of product */
  void setVolumeStartTime(const Time& t){ myVolStartTime = t; }

  /** Get generation time of product */
  Time
  getGenTime() const { return myGenTime; }

  /** Set generation time of product */
  void setGenTime(const Time& t){ myGenTime = t; }

  /** Get the data threshold value table */
  std::array<short, 16> getEncodedThresholds(){ return myDataThresholds; }

  /** Gets real values of thresholds after decoding
   *@param   decoded_thresholds a vector of float which will be filled in
   *				  decoded thresholds
   */
  void
  getDecodedThresholds(std::vector<float>& decoded_thresholds) const;

  /** Get dependent value 1 to 10  (1-based) */
  short getDep(size_t i){ return myDeps[i - 1]; }

  /** Set dependent value 1 to 10  (1-based) */
  void setDep(size_t i, short value){ myDeps[i - 1] = value; }

  /** Get the elevation number */
  short getElevationNum(){ return myElevNum; }

  /** Set the elevation number */
  void setElevationNum(short e){ myElevNum = e; }

  /** Get the number of maps */
  short getNumberMaps(){ return myNumMaps; }

  /** Set the number of maps */
  void setNumberMaps(short m){ myNumMaps = m; }

  /** Get the write size of this block.  Most are static. */
  size_t
  size() const { return 102; }

protected:

  LLH myLocation;      ///< Location of product
  short myProductCode; ///< Product code
  short myOpMode;      ///< Operational mode
  short myVCP;         ///< Volume Coverage Pattern
  short mySeqNumber;   ///< Sequence number
  short myVolScanNum;  ///< Volume scan number
  Time myVolStartTime; ///< Volume start time
  Time myGenTime;      ///< Product generation time

public:                               // temp for writing
  std::array<short, 10> myDeps = { }; ///< Product Dependent (p1 to p10)

protected:
  short myElevNum; ///< Elevation number
public:

  std::array<short, 16> myDataThresholds = { }; ///< Data Level Threshold

protected:
  short myNumMaps; ///< Number of maps
public:
  int mySymbologyOffset; ///< Offset to symbology
  int myGraphicOffset;   ///< Offset to graphic
  int myTabularOffset;   ///< Offset to tabular

public:

  /** Decode method 1 */
  void
  decodeMethod1(std::vector<float>& a) const;

  /** Set decode method 2 for writing alpha */
  void
  setDecodeMethod2(float scale, float offset,
    int leading_flags, int size);

  /** Decode method 2 */
  void
  decodeMethod2(std::vector<float>& a) const;

  /** Decode method 3 */
  void
  decodeMethod3(std::vector<float>& a, int min, int delta, int num_data_level) const;

  /** Decode method 4 */
  void
  decodeMethod4(std::vector<float>& a, int min, int delta, int num_data_level) const;

  /** Decode method 5 */
  void
  decodeMethod5(std::vector<float>& a, int min, int delta, int num_data_level) const;

  /** Decode method 6 */
  void
  decodeMethod6(std::vector<float>& a, int min, int delta, int num_data_level) const;

  /** Decode method 7 */
  void
  decodeMethod7(std::vector<float>& a, int min, int delta, int num_data_level) const;

  /** Helper function used only by DecodeThresholds() */
  float
  getValue(short int a) const;

  /**
   * Decodes threshold according to ICD
   *
   * @param threshold     two bytes value
   * @return a float which is the real raw value
   */
  float
  DecodeThresholds(short int threshold) const;
};
}
