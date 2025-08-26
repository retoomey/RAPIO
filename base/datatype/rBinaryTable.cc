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
