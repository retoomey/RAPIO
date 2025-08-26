#include "rFusionBinaryTable.h"

#include "rError.h"
#include "rBinaryIO.h"
#include "rStrings.h"
#include "rError.h"

#include <iomanip>

using namespace rapio;
using namespace std;

size_t FusionBinaryTable::BLOCK_LEVEL;

bool FusionBinaryTable::myStreamRead = false;

FusionBinaryTable::~FusionBinaryTable()
{
  // Ensure any open stream file is closed
  if (myFile != 0) {
    fclose(myFile);
    myFile = 0;
  }
}

void
FusionBinaryTable::getBlockLevels(std::vector<std::string>& levels)
{
  // Level stack
  BinaryTable::getBlockLevels(levels);
  levels.push_back("F");
  FusionBinaryTable::BLOCK_LEVEL = levels.size();
}

bool
FusionBinaryTable::readBlock(const std::string& path, FILE * fp)
{
  // Blocks have to be ordered by magic string
  // If superclass was able to read its block, then...
  if (BinaryTable::readBlock(path, fp) &&
    matchBlockLevel(FusionBinaryTable::BLOCK_LEVEL))
  {
    // More header for us....
    // FIXME: We 'could' generalize all DataType attributes by storing name/type and count.
    // But that would make larger files.  We'll just do this for the moment.  Maybe a
    // subclass that handles general DataGrids could be useful?
    // I'm curious how much faster than netcdf we'd be doing it

    // Adding a version number for future use
    BinaryIO::read_type<size_t>(myVersionID, fp);
    if (myVersionID != Version) {
      LogSevere("Error reading raw file.  Version is unknown " << myVersionID << "\n");
      return false;
    }

    std::string radarName, typeName, units;
    long xBase = 0, yBase = 0;
    BinaryIO::read_type<char>(myMissingMode, fp);
    BinaryIO::read_string8(radarName, fp);
    setString("Radarname", radarName);
    BinaryIO::read_string8(typeName, fp);
    setString("Typename", typeName);
    BinaryIO::read_string8(units, fp);
    setUnits(units);
    BinaryIO::read_type<long>(xBase, fp);
    setLong("xBase", xBase);
    BinaryIO::read_type<long>(yBase, fp);
    setLong("yBase", yBase);

    // LLH
    double lat, lon;
    float ht;
    BinaryIO::read_type<double>(lat, fp);
    BinaryIO::read_type<double>(lon, fp);
    BinaryIO::read_type<float>(ht, fp);
    setLocation(LLH(lat, lon, ht));

    // Time
    time_t t;
    double f;
    BinaryIO::read_type<time_t>(t, fp);
    BinaryIO::read_type<double>(f, fp);
    setTime(Time(t, f));

    // Read the sizes of the value/missing arrays
    BinaryIO::read_type<size_t>(myValueSize, fp);
    BinaryIO::read_type<size_t>(myMissingSize, fp);

    // If not streaming, store in our internal fields (which can be large),
    // otherwise we wait for a get() call
    if (!myStreamRead) {
      myDataPosition = 0; // which marks it since we always have a header

      myXs.resize(myValueSize);
      myYs.resize(myValueSize);
      myZs.resize(myValueSize);
      myNums.resize(myValueSize);
      myDems.resize(myValueSize);
      myXMissings.resize(myMissingSize);
      myYMissings.resize(myMissingSize);
      myZMissings.resize(myMissingSize);
      myLMissings.resize(myMissingSize);
      for (size_t i = 0; i < myValueSize; ++i) {
        BinaryIO::read_type<short>(myXs[i], fp);
        BinaryIO::read_type<short>(myYs[i], fp);
        BinaryIO::read_type<char>(myZs[i], fp);
        BinaryIO::read_type<float>(myNums[i], fp);
        BinaryIO::read_type<float>(myDems[i], fp);
      }
      for (size_t i = 0; i < myMissingSize; ++i) {
        BinaryIO::read_type<short>(myXMissings[i], fp);
        BinaryIO::read_type<short>(myYMissings[i], fp);
        BinaryIO::read_type<char>(myZMissings[i], fp);
        BinaryIO::read_type<short>(myLMissings[i], fp);
      }

      /*
       *  // The data fields
       *  BinaryIO::read_vector(myXs, fp);
       *  BinaryIO::read_vector(myYs, fp);
       *  BinaryIO::read_vector(myZs, fp);
       *  BinaryIO::read_vector(myNums, fp);
       *  BinaryIO::read_vector(myDems, fp);
       *
       *  BinaryIO::read_vector(myXMissings, fp);
       *  BinaryIO::read_vector(myYMissings, fp);
       *  BinaryIO::read_vector(myZMissings, fp);
       *  BinaryIO::read_vector(myLMissings, fp);
       */
    } else {
      myDataPosition = ftell(fp);
      myFilePath     = path;
      myFile         = 0;
      myValueAt      = 0;
      myMissingAt    = 0;
      myRLECounter   = 0;
    }
    return true;
  }
  return false;
} // FusionBinaryTable::readBlock

bool
FusionBinaryTable::get(float& n, float& d, short& x, short& y, short& z)
{
  // We didn't read in streaming mode
  if (myDataPosition < 1) {
    return false;
  }

  // First time, try to reopen the file...
  if (myFile == 0) {
    myFile = fopen(myFilePath.c_str(), "rb");
    if (fseek(myFile, myDataPosition, (SEEK_SET != 0))) {
      LogSevere("Couldn't read at position " << myDataPosition << "...\n");
      fclose(myFile);
      myFile = 0;
      return false;
    }
  }

  // First read the non-missing values...
  if (myValueSize > 0) {
    if (myValueAt < myValueSize) {
      BinaryIO::read_type<short>(x, myFile);
      BinaryIO::read_type<short>(y, myFile);
      char zc;
      BinaryIO::read_type<char>(zc, myFile);
      z = zc;
      BinaryIO::read_type<float>(n, myFile);
      BinaryIO::read_type<float>(d, myFile);
      myValueAt++;
      return true;
    }
  }

  // skip missing for moment
  // ...then read the missing values from RLE arrays and expand
  if (myMissingSize > 0) {
    if (myMissingAt < myMissingSize) {
      n = Constants::MissingData;
      d = 1.0; // doesn't matter weighted average ignores missing

      // Run length encoding expansion I goofed a few times,
      // so extra description here:
      // lengths: 5, 3
      // values:  1  2
      // --> 1 1 1 1 1 2 2 2  Expected output
      //     0 1 2 3 4 0 1 2  RLECounter
      //     0 0 0 0 0 1 1 1  MCounter
      //     5 5 5 5 5 3 3 3  l (current length)

      // If the first in the sequence...
      if (myRLECounter == 0) {
        // Read the current RLE block
        BinaryIO::read_type<short>(myXBlock, myFile);
        BinaryIO::read_type<short>(myYBlock, myFile);
        BinaryIO::read_type<char>(myZBlock, myFile);
        BinaryIO::read_type<short>(myLengthBlock, myFile);
      }
      x = myXBlock + myRLECounter;
      y = myYBlock;
      z = myZBlock;
      // Update for the next missing, if any
      myRLECounter++;
      if (myRLECounter >= myLengthBlock) { // overflow
        myRLECounter = 0;
        myMissingAt++;
      }
      return true;
    }
  }
  // Nothing left...close file
  fclose(myFile);
  myFile = 0;
  return false;
} // FusionBinaryTable::get

bool
FusionBinaryTable::writeBlock(FILE * fp)
{
  if (BinaryTable::writeBlock(fp)) {
    // ----------------------------------------------------------
    // Write our data block if available (match with read above)

    // Header (attributes)
    // FIXME: We 'could' generalize all DataType attributes by storing name/type and count.
    // But that would make larger files.  We'll just do this for the moment.  Maybe a
    // subclass that handles general DataGrids could be useful?

    // Adding a version number for future use
    BinaryIO::write_type<size_t>(myVersionID, fp);

    std::string radarName, typeName, units;
    BinaryIO::write_type<char>(myMissingMode, fp);
    getString("Radarname", radarName);
    getString("Typename", typeName);
    units = getUnits();
    long xBase = 0, yBase = 0;
    getLong("xBase", xBase);
    getLong("yBase", yBase);

    LLH center = getLocation(); // next
    Time t     = getTime();

    // Write it
    BinaryIO::write_string8(radarName, fp);
    BinaryIO::write_string8(typeName, fp);
    BinaryIO::write_string8(units, fp);
    BinaryIO::write_type<long>(xBase, fp);
    BinaryIO::write_type<long>(yBase, fp);

    // Write LLH
    BinaryIO::write_type<double>(center.getLatitudeDeg(), fp);
    BinaryIO::write_type<double>(center.getLongitudeDeg(), fp);
    BinaryIO::write_type<float>(center.getHeightKM(), fp);

    // Write Time
    BinaryIO::write_type<time_t>(t.getSecondsSinceEpoch(), fp);
    BinaryIO::write_type<double>(t.getFractional(), fp);

    // Write the sizes of the value/missing arrays
    myValueSize   = myXs.size();
    myMissingSize = myXMissings.size();
    BinaryIO::write_type<size_t>(myValueSize, fp);
    BinaryIO::write_type<size_t>(myMissingSize, fp);

    // The data fields.  Ok we wanna stream them back to save ram,
    // so make the order streamable...
    // which is probably slower.  We'll probably have to make blocks
    // also this won't compress.  Kill me.
    // FIXME: We'll probably have to block and byte-align and compress
    for (size_t i = 0; i < myXs.size(); ++i) {
      BinaryIO::write_type<short>(myXs[i], fp);
      BinaryIO::write_type<short>(myYs[i], fp);
      BinaryIO::write_type<char>(myZs[i], fp);
      BinaryIO::write_type<float>(myNums[i], fp);
      BinaryIO::write_type<float>(myDems[i], fp);
    }
    for (size_t i = 0; i < myXMissings.size(); ++i) {
      BinaryIO::write_type<short>(myXMissings[i], fp);
      BinaryIO::write_type<short>(myYMissings[i], fp);
      BinaryIO::write_type<char>(myZMissings[i], fp);
      BinaryIO::write_type<short>(myLMissings[i], fp);
    }

    /*
     *  BinaryIO::write_vector("X", myXs, fp);
     *  BinaryIO::write_vector("Y", myYs, fp);
     *  BinaryIO::write_vector("Z", myZs, fp);
     *  BinaryIO::write_vector("N", myNums, fp);
     *  BinaryIO::write_vector("D", myDems, fp);
     *
     *  BinaryIO::write_vector("Xm", myXMissings, fp);
     *  BinaryIO::write_vector("Ym", myYMissings, fp);
     *  BinaryIO::write_vector("Zm", myZMissings, fp);
     *  BinaryIO::write_vector("Lm", myLMissings, fp);
     */

    return true;
  }
  return false;
} // FusionBinaryTable::writeBlock

bool
FusionBinaryTable::dumpToText(std::ostream& o)
{
  const std::string i = "\t";

  o << "RAPIO FusionBinaryTable (Used by Fusion)\n";

  auto& t = *this;

  // ---------------------------------------------------------
  //  RObsBinaryTable stuff...
  std::string r;
  long v;

  o << "Header:\n";
  t.getString("Radarname", r);
  o << i << "RadarName: '" << r << "'\n";
  t.getString("Typename", r);
  o << i << "Typename: '" << r << "'\n";
  r = t.getUnits();
  o << i << "Units: '" << r << "'\n";
  t.getLong("xBase", v);
  o << i << "xBase: '" << v << "'\n";
  t.getLong("yBase", v);
  o << i << "yBase: '" << v << "'\n";
  bool flag =t.getUseMissingAsUnavailable();
  o << i << "NoMissing: '" << (flag? "On":"Off") << "'\n";
  o << i << "Time: '" << t.getTime() << "'\n";
  o << i << "Center: '" << t.getLocation() << "'\n";
  o << "Data:\n";
  o << i << "X:  " << myXs.size() << "\n";
  o << i << "Y:  " << myYs.size() << "\n";
  o << i << "Z:  " << myZs.size() << "\n";
  o << i << "N:  " << myNums.size() << "\n";

  o << i << "D:  " << myDems.size() << "\n";
  o << i << "Xm: " << myXMissings.size() << "\n";
  o << i << "Ym: " << myYMissings.size() << "\n";
  o << i << "Zm: " << myZMissings.size() << "\n";
  o << i << "Lm: " << myLMissings.size() << "\n";

  // Dump the values
  size_t perRow = 0;
  size_t ipr    = 3;

  //  o << std::fixed << std::setprecision(2);
  o << "0: ";
  for (size_t r = 0; r < myXs.size(); ++r) {
    o << "(" << myXs[r] << "," << myYs[r] << "," << (int) myZs[r] << ", [" << myNums[r] << "/" << myDems[r] << "])";
    // o << "("<<myXs[r]<<","<<myYs[r]<<","<<(int)(myZs[r])<<" ;
    if (++perRow > ipr) {
      o << "\n" << r << ":";
      perRow = 0;
    } else {
      o << ",";
    }
  }
  o << "\n";
  o << "-------------------\n";
  perRow = 0;
  ipr    = 5;
  for (size_t r = 0; r < myXMissings.size(); ++r) {
    o << "(" << myXMissings[r] << "," << myYMissings[r] << "," << (int) (myZMissings[r]) << "," << myLMissings[r] <<
      ")";
    if (++perRow > ipr) {
      o << "\n" << r << ":";
      perRow = 0;
    } else {
      o << ",";
    }
  }
  o << "\n";
  o << std::defaultfloat;

  return true;
} // FusionBinaryTable::dumpToText
