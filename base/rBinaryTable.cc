#include "rBinaryTable.h"

#include "rError.h"
#include "rBinaryIO.h"
#include "rStrings.h"

using namespace rapio;
using namespace std;

// Bleh we _really_ need to check the signature, BEFORE
// creating the object.  Thus a factory.
const size_t BinaryTable::BLOCK_LEVEL = 1;

// Subclasses of BinaryTable
size_t WObsBinaryTable::BLOCK_LEVEL;
size_t RObsBinaryTable::BLOCK_LEVEL;

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
BinaryTable::readBlock(FILE * fp)
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

  std::string test2;

  BinaryIO::read_string8(test2, fp);

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

  std::string test2 = "Toomey was here";

  BinaryIO::write_string8(test2, fp);

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
WObsBinaryTable::readBlock(FILE * fp)
{
  // Blocks have to be ordered by magic string
  // If superclass was able to read its block, then...
  if (BinaryTable::readBlock(fp) &&
    matchBlockLevel(WObsBinaryTable::BLOCK_LEVEL))
  {
    // ----------------------------------------------------------
    // Read our data block if available (match with write below)
    BinaryIO::read_string8(typeName, fp);
    myTypeName = typeName;

    // Handle units
    std::string unit;
    BinaryIO::read_string8(unit, fp);
    setDataAttributeValue("Unit", "dimensionless", unit);
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
      size_t aSize = markedLines.size();
      LogInfo("SIZE OF LINES IS " << aSize << "\n");
      if (aSize > 5) { aSize = 5; }
      for (int i = 0; i < aSize; i++) {
        std::cout << "      line: " << i << "(" << markedLines[i].len << ") ( " << markedLines[i].x << ","
                  << markedLines[i].y << "," << markedLines[i].z << ")\n";
      }
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

void
RObsBinaryTable::getBlockLevels(std::vector<std::string>& levels)
{
  // Level stack
  WObsBinaryTable::getBlockLevels(levels);
  levels.push_back("R");
  RObsBinaryTable::BLOCK_LEVEL = levels.size();
}

bool
RObsBinaryTable::readBlock(FILE * fp)
{
  // Blocks have to be ordered by magic string
  // If superclass was able to read its block, then...
  if (WObsBinaryTable::readBlock(fp) &&
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
