#pragma once

#include <rBinaryTable.h>
#include <string>
#include <vector>

namespace rapio {
/** Binary table for fusion (netcdf is averaging 0.5 to 1.8 sec per read) */
class FusionBinaryTable : public BinaryTable
{
public:

  /** Create a fusion binary table */
  FusionBinaryTable()
    : myVersionID(Version), myMissingMode(0), myValueSize(0), myMissingSize(0), myDataPosition(0), myFile(0),
    myValueAt(0), myMissingAt(0), myRLECounter(0),
    myXBlock(0), myYBlock(0), myZBlock(0), myLengthBlock(0)
  {
    // Lookup for read/write factories
    myDataType = "FusionBinaryTable";
  }

  /** Destroy a FusionBinaryTable, ensuring any open files are closed.
   * This is important especially for the streaming mode that doesn't
   * necessarily finish reading. */
  ~FusionBinaryTable();

  /** Read as a stream using get() vs reading into storage.  This
   * is for reading massive datasets.  The default is false  */
  static bool myStreamRead;

  /** Non-stream add ability (possibly could stream as well but need API changes) */
  inline void
  add(float& n, float& d, short& x, short& y, short& z)
  {
    myNums.push_back(n);
    myDems.push_back(d);
    myXs.push_back(x); // X represents LON
    myYs.push_back(y); // Y represents LAT
    myZs.push_back(z); // Z represents HEIGHT.  We're hiding that z is a char
    myValueSize++;
  }

  /** Non-stream add RLE missing ability */
  inline void
  addMissing(size_t& x, size_t& y, size_t& z, size_t& l)
  {
    myXMissings.push_back(x); // reducing size_t to short...
    myYMissings.push_back(y);
    myZMissings.push_back(z);
    myLMissings.push_back(l);
    myMissingSize++;
  }

  /** Stream read ability */
  bool
  get(float& n, float& d, short& x, short& y, short& z);

  /** Read our block from file if it exists at current location */
  virtual bool
  readBlock(const std::string& path, FILE * fp) override;

  /** Write our block to file at current location */
  virtual bool
  writeBlock(FILE * fp) override;

  /** Our block level...which is level 1 and the first always.  Every subclass
   * should increase their count by 1 (depth of subclass tree.  Siblings have same value) */
  static size_t BLOCK_LEVEL;

  /** Get the block level magic vector for this class. */
  virtual void
  getBlockLevels(std::vector<std::string>& levels) override;

  /** Send human readable output to a ostream.  This is
   * called by iotext and rdump to view the file */
  virtual bool
  dumpToText(std::ostream& s) override;

  /** Number of values stored (non-missing) */
  size_t getValueSize(){ return myValueSize; }

  /** Number of missing stored */
  size_t getMissingSize(){ return myMissingSize; }

  /** Get missing mode. Using this to toggle background mask
   * application.  Using these values for moment:
   * FIXME: Could enum it all, etc.  Might come to that.
   * 0 -- Missing is background.  Missing values replace old data.
   * 1 -- DataUnavailable is background.  This replaces old data.
   */
  char getMissingMode(){ return myMissingMode; }

  /** Return use missing as unavailable */
  bool
  getUseMissingAsUnavailable()
  {
    return (myMissingMode == 1);
  }

  /** Set the missing mode, current just to 1 (the nomissing flag) */
  void
  setUseMissingAsUnavailable()
  {
    myMissingMode = 1;
  }

protected:
  /** Current version of the Fusion Binary Table */
  static constexpr size_t Version = 1;

  char myMissingMode;   ///< Current way missing handled
  size_t myVersionID;   ///< Current version ID
  size_t myValueSize;   ///< Sizes of X,Y,Z value storage
  size_t myMissingSize; ///< Sizes of missing value storage

  // State information for streaming
  long myDataPosition;    ///< Position in file for streaming mode
  std::string myFilePath; ///< File path for streaming mode
  FILE * myFile;          ///< FILE pointer for streaming mode
  size_t myValueAt;       ///< Stream count of values
  size_t myMissingAt;     ///< Stream count of missings

  size_t myRLECounter; ///< Current location in RLE block
  short myXBlock;      ///< X current
  short myYBlock;      ///< Y current
  char myZBlock;       ///< Z current
  short myLengthBlock; ///< Length current


public:
  // Data stored (ONLY if self contained. With fusion data is so large we don't
  // want to double buffer/etc.  We use streamer mode with get() and these are ignored.
  // this is especially important during reading in fusion stage2 where memory is at a premium.
  std::vector<float> myNums;      ///< Numerators for global weighted average
  std::vector<float> myDems;      ///< Denominators for global weighted averag
  std::vector<short> myXs;        ///< X location for a true value
  std::vector<short> myYs;        ///< Y location for a true value
  std::vector<char> myZs;         ///< Z value, height for a true value
  std::vector<short> myXMissings; ///< X start for RLE missing range
  std::vector<short> myYMissings; ///< Y start for RLE missing range
  std::vector<char> myZMissings;  ///< Z start for RLE missing range
  std::vector<short> myLMissings; ///< The length of the run in X horizontal
};
}
