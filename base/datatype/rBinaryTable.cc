#include "rBinaryTable.h"

#include "rError.h"
#include "rBinaryIO.h"
#include "rStrings.h"

#include <iomanip>

using namespace rapio;
using namespace std;

// Bleh we _really_ need to check the signature, BEFORE
// creating the object.  Thus a factory.
const size_t BinaryTable::BLOCK_LEVEL = 1;

// Subclasses of BinaryTable
size_t WObsBinaryTable::BLOCK_LEVEL;
size_t RObsBinaryTable::BLOCK_LEVEL;
size_t FusionBinaryTable::BLOCK_LEVEL;

bool FusionBinaryTable::myStreamRead = false;

BinaryTable::BinaryTable() : myLastFileVersion(0)
{
  setDataType("BinaryTable");
  // Current the default write for all binary tables is raw which makes sense
  setReadFactory("raw");
}

void
BinaryTable::getBlockLevels(std::vector<std::string>& levels)
{
  // The root level...every binary file will have this
  // Subclasses should call this and then append their own unique block level
  levels.push_back("W2");

  // SUBCLASS::BLOCK_LEVEL = levels.size();
}

size_t
BinaryTable::getVersion()
{
  // Version number to change on major changes.  Subclasses
  // can use to verify readability.
  return (1); // Version 1 Jan 2018
}

size_t
BinaryTable::getLastFileVersion()
{
  // Version number of last read file.  Subclasses could use this
  // to handle older version for compatibility.
  return (myLastFileVersion);
}

bool
BinaryTable::canHandleVersion(size_t version)
{
  const size_t ourVersion = getVersion();

  if (version > ourVersion) {
    return (false);
  }
  return (true);
}

bool
BinaryTable::matchBlockLevel(size_t level)
{
  // Virtual.  Get the FULL available block level stack
  // based on current subclass we are
  std::vector<std::string> classLevels;

  getBlockLevels(classLevels);

  // We need to match every one up to the level we ask for...
  const size_t classLevelSize = classLevels.size();
  const size_t fileLevelSize  = myLastFileBlockLevels.size();

  if ((level > fileLevelSize) || (level > classLevelSize)) {
    return (false); // We don't have this level's data obviously
  }

  if (level == 0) {
    return (false); // Level base is 1
  }

  // Match every level top down...
  bool match = true;

  level--; // shift to zero base

  for (size_t i = 0; i < level; i++) {
    if (classLevels[i] != myLastFileBlockLevels[i]) {
      match = false;
      break;
    }
  }
  return (match);
}

void
BinaryTable::blockLevelsToMagic(
  const std::vector<std::string>& levels,
  std::string                   & magic)
{
  const size_t aSize = levels.size();

  std::stringstream s;

  for (size_t i = 0; i < aSize; i++) {
    s << levels[i];

    if (i != aSize - 1) {
      s << "-";
    }
  }
  magic = s.str();
}

void
BinaryTable::magicToBlockLevels(
  const std::string       & source,
  std::vector<std::string>& blocks)
{
  // We might read a bad file and have some crazy string...so we'll
  // do some sanity first...
  if (source.size() > 1000) {
    LogSevere("Magic string for file seems suspicious, probably bad data...\n");
  } else {
    blocks.clear();
    Strings::split(source, '-', &blocks);
  }
}

bool
BinaryTable::readBlock(const std::string& path, FILE * fp)
{
  // 1. Read the magic marker name...
  std::string disk_magic;

  BinaryIO::read_string8(disk_magic, fp);
  magicToBlockLevels(disk_magic, myLastFileBlockLevels);

  // Check block level...this is the base class, so
  // every raw file MUST have our level (The "W2-" part basically)
  // and also a bad file, etc would catch here quickly
  if (!matchBlockLevel(BinaryTable::BLOCK_LEVEL)) {
    LogSevere("This file doesn't have a marker as a WDSS2 raw file\n");
    myLastFileVersion = 0;
    return (false);
  }

  // 2. Read the file version number...
  size_t file_version;

  BinaryIO::read_type<size_t>(file_version, fp);
  myLastFileVersion = file_version;

  // Check the file version number....
  if (!canHandleVersion(file_version)) {
    const size_t ourVersion = getVersion();
    LogSevere("Trying to read a binary file with version number "
      << file_version << " which is too new vs our version of " << ourVersion
      << "You need a newer build.\n");
    return (false);
  }

  // Get the full data type (specialization)
  BinaryIO::read_string8(myDataType, fp);

  return (true);
} // BinaryTable::readBlock

bool
BinaryTable::writeBlock(FILE * fp)
{
  // 1. Write the magic marker name...
  std::string magic;
  std::vector<std::string> classLevels;

  getBlockLevels(classLevels); // Virtual so full subclass name
  blockLevelsToMagic(classLevels, magic);
  BinaryIO::write_string8(magic, fp);

  // 2. Write the file version number...
  const size_t version = getVersion();

  BinaryIO::write_type<size_t>(version, fp);

  // 3. Write a data type always.  We can use this to create the
  // proper class on read.  Makes the whole block thing kinda silly
  // but we need to be able to back read MRMS at least for now
  // "Toomey was here" still in MRMS, we'll assume RObsBinaryTable;

  BinaryIO::write_string8(myDataType, fp);

  return (true);
}

// Weighted  ----------------------------------------
//
void
WObsBinaryTable::getBlockLevels(std::vector<std::string>& levels)
{
  // Level stack
  BinaryTable::getBlockLevels(levels);
  levels.push_back("W");
  WObsBinaryTable::BLOCK_LEVEL = levels.size();
}

bool
WObsBinaryTable::readBlock(const std::string& path, FILE * fp)
{
  // Blocks have to be ordered by magic string
  // If superclass was able to read its block, then...
  if (BinaryTable::readBlock(path, fp) &&
    matchBlockLevel(WObsBinaryTable::BLOCK_LEVEL))
  {
    // ----------------------------------------------------------
    // Read our data block if available (match with write below)
    BinaryIO::read_string8(typeName, fp);
    myTypeName = typeName;

    // Handle units
    std::string unit;
    BinaryIO::read_string8(unit, fp);
    setUnits(unit);

    BinaryIO::read_type<float>(lat, fp);
    BinaryIO::read_type<float>(lon, fp);
    BinaryIO::read_type<float>(ht, fp);
    myLocation = LLH(lat, lon, ht / 1000.0);
    BinaryIO::read_type<time_t>(data_time, fp);
    BinaryIO::read_type<time_t>(valid_time, fp);

    LogInfo(
      "RAW INFO: " << typeName << "/" << unit << "," << lat << "," << lon << "," << ht << "," << data_time << "," << valid_time
                   << "\n");

    // Handle the marked lines reading in...
    BinaryIO::read_string8(markedLinesCacheFile, fp);
    std::vector<Line> markedLines;
    if (markedLinesCacheFile == "") {
      BinaryIO::read_vector(markedLines, fp);
    } else {
      LogSevere("RAWERROR: RAW File wants us to read a cached file, not supported.\n");
    }

    // Read the six default arrays that always read
    BinaryIO::read_vector(x, fp);
    BinaryIO::read_vector(y, fp);
    BinaryIO::read_vector(z, fp);
    BinaryIO::read_vector(newvalue, fp);
    BinaryIO::read_vector(scaled_dist, fp);
    BinaryIO::read_vector(elevWeightScaled, fp);

    // ----------------------------------------------------------
    return true;
  } else {
    LogSevere("WObsBinaryTable Missing our data block in .raw file\n");
  }
  return false;
} // WObsBinaryTable::readBlock

bool
WObsBinaryTable::writeBlock(FILE * fp)
{
  if (BinaryTable::writeBlock(fp)) {
    // ----------------------------------------------------------
    // Write our data block if available (match with read above)

    // Header
    BinaryIO::write_string8(typeName, fp);

    // Handle units
    std::string unit = getUnits();
    BinaryIO::write_string8(unit, fp);

    BinaryIO::write_type<float>(lat, fp);
    BinaryIO::write_type<float>(lon, fp);
    BinaryIO::write_type<float>(ht, fp);
    BinaryIO::write_type<time_t>(data_time, fp);
    BinaryIO::write_type<time_t>(valid_time, fp);

    // Handle the marked lines writing out...
    BinaryIO::write_string8(markedLinesCacheFile, fp);
    if (markedLinesCacheFile == "") {
      BinaryIO::write_vector("Marked", markedLines, fp);
    }

    // Write the six default arrays that always write
    BinaryIO::write_vector("X", x, fp);
    BinaryIO::write_vector("Y", y, fp);
    BinaryIO::write_vector("Z", z, fp);
    BinaryIO::write_vector("V", newvalue, fp);
    BinaryIO::write_vector("Scaled", scaled_dist, fp);
    BinaryIO::write_vector("ElevScaled", elevWeightScaled, fp);
    // ----------------------------------------------------------
    return true;
  } else {
    // No need to error, weighted already did...
  }
  return false;
} // WObsBinaryTable::writeBlock

void
RObsBinaryTable::getBlockLevels(std::vector<std::string>& levels)
{
  // Level stack
  WObsBinaryTable::getBlockLevels(levels);
  levels.push_back("R");
  RObsBinaryTable::BLOCK_LEVEL = levels.size();
}

bool
RObsBinaryTable::readBlock(const std::string& path, FILE * fp)
{
  // Blocks have to be ordered by magic string
  // If superclass was able to read its block, then...
  if (WObsBinaryTable::readBlock(path, fp) &&
    matchBlockLevel(RObsBinaryTable::BLOCK_LEVEL))
  {
    // ----------------------------------------------------------
    // Read our data block if available (match with write below)

    // More header for us....
    BinaryIO::read_string8(radarName, fp);
    BinaryIO::read_type<int>(vcp, fp);    // fread( &vcp, sizeof(int), 1, fp );
    BinaryIO::read_type<float>(elev, fp); // fread( &elev, sizeof(float), 1, fp );

    // And our arrays
    BinaryIO::read_vector(azimuth, fp);
    BinaryIO::read_vector<mrmstime>(aztime, fp);

    // ----------------------------------------------------------
    return true;
  } else {
    LogSevere("RObsBinaryTable: Missing our data in .raw file\n");
  }
  return false;
}

bool
RObsBinaryTable::writeBlock(FILE * fp)
{
  // Blocks have to be ordered by magic string
  if (WObsBinaryTable::writeBlock(fp)) {
    // ----------------------------------------------------------
    // Write our data block if available (match with read above)

    // Header...
    BinaryIO::write_string8(radarName, fp);
    BinaryIO::write_type<int>(vcp, fp);    // fwrite( &vcp, sizeof(int), 1, fp );
    BinaryIO::write_type<float>(elev, fp); // fwrite( &elev, sizeof(float), 1, fp );

    // Data...
    BinaryIO::write_vector("Azimuth", azimuth, fp);
    BinaryIO::write_vector("Aztime", aztime, fp);

    // ----------------------------------------------------------
    return true;
  } else {
    // No need to error, weighted already did...
  }
  return false;
}

bool
RObsBinaryTable::dumpToText(std::ostream& o)
{
  const std::string i = "\t";

  o << "MRMS RObsBinaryTable (Used by w2merger)\n";

  auto& t = *this;

  // ---------------------------------------------------------
  //  RObsBinaryTable stuff...
  o << i << "RadarName: " << t.radarName << "\n"; // RObsBinaryTable
  o << i << "VCP: " << t.vcp << "\n";             // RObsBinaryTable
  o << i << "Elevation: " << t.elev << "\n";      // RObsBinaryTable
  o << i << "Units: " << t.getUnits() << "\n";    // DataType

  // WObsBinaryTable.  FIXME: probably don't need multiple classes anymore?
  o << i << "TypeName: " << t.typeName << "\n";
  o << i << "MarkedLinesCacheFile: " << t.markedLinesCacheFile << "\n";
  o << i << "MarkedLinesInternalSize: " << t.markedLines.size() << "\n";
  o << i << "Latitude: " << t.lat << " Degrees.\n";
  o << i << "Longitude: " << t.lon << " Degrees.\n";
  o << i << "Height: " << t.ht << " Kilometers.\n";
  o << i << "Time: " << t.data_time << " Epoch.\n";
  o << i << "Valid Time: " << t.valid_time << " Seconds.\n";
  o << "\n------------------------------\n";

  // Marked lines for WObsBinarytable
  size_t aSize = markedLines.size();

  o << i << "Marked line array size: " << aSize << "\n";
  if (aSize > 5) { aSize = 5; }
  for (int i = 0; i < aSize; i++) {
    o << "  Line: " << i << "(" << markedLines[i].len << ") ( " << markedLines[i].x << ","
      << markedLines[i].y << "," << markedLines[i].z << ")\n";
  }

  // Arrays for WObsBinaryTable
  // Ok do we dump per point, or do separate arrays like ncdump?
  // Could be a key option passed in..
  // RObsBinaryTable adds azimuth, aztime
  size_t s = t.x.size();

  o << i << "Points: " << s << "\n";
  if (s > 100) { s = 100; } // For now dump just s points for debugging
  o << i << "Sampling first " << s << " data points:\n";

  // Dumping info per point vs per array.  Having a toggle even with ncdump would
  // be a nice ability
  auto& x = t.x; // WObs
  auto& y = t.y;
  auto& z = t.z;

  for (size_t j = 0; j < s; j++) {
    o << j << ":\n";
    o << i << x[j] << ", " << y[j] << ", " << z[j] << " (xyz)\n";
    o << i << t.newvalue[j] << ", " << t.scaled_dist[j] << ", " << (int) t.elevWeightScaled[j] <<
      " (value, w1-dist, w2-elev)\n";
    // Only RObs:
    o << i << t.azimuth[j] << ", " << t.aztime[j].epoch_sec << "." << t.aztime[j].frac_sec << ", " <<
      "(az, aztime)\n";
  }

  return true;
} // RObsBinaryTable::dumpToText

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

    std::string radarName, typeName, units;
    long xBase = 0, yBase = 0;
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

    LLH center;
    Time t;
    BinaryIO::read_type<LLH>(center, fp);
    setLocation(center);
    BinaryIO::read_type<Time>(t, fp);
    setTime(t);

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
    std::string radarName, typeName, units;
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
    BinaryIO::write_type<LLH>(center, fp);
    BinaryIO::write_type<Time>(t, fp);

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
