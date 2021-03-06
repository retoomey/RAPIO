#pragma once

#include <rDataType.h>
#include <rLLH.h>
#include <rTime.h>
#include <string>
#include <vector>

namespace rapio {
/** Binary table has a metadata field and columns of binary data
 * that can vary in what they store.  I'm migrating this storage
 * from merger and enhancing it in allow to other algorithms to
 * use this type of format.
 *
 * All binary table writes to disk is the magic string identifying
 * the writing class and a file version number we hard code for
 * backward compatibility. Subclasses append to this data.
 * Levels are from a string
 * in the file of the form W2-Level2-Level3, etc.  These represent the
 * subclass tree of the writing class, where each level is a 'block' of
 * data.  This allows dynamic expandsion of files.
 *
 *
 */
class BinaryTable : public DataType {
public:

  BinaryTable() : myLastFileVersion(0)
  {
    // Lookup for read/write factories
    myDataType = "BinaryTable";
  }

  /** Get the block level magic vector for this class.  Subclasses MUST override
   * to call their
   * superclass and then push back their level identifier. */
  virtual void
  getBlockLevels(std::vector<std::string>& levels);

  /** Return the version number of binary format.  Change on a
   * major change.  This will allow backward compatibility */
  size_t
  getVersion();

  /** Return the version number of last read file, or zero if not */
  size_t
  getLastFileVersion();

  /** Can we handle this version?  Default handles version
   * less than or equal to current version */
  virtual bool
  canHandleVersion(size_t version);

  // ----------------------------------------------------------------------------
  // To magic string and back...
  //

  /** Convert a magic string such as "W2-W-R" to block levels "W2, "W", "R" */
  static void
  magicToBlockLevels(const std::string & magic,
    std::vector<std::string>           & blocks);

  /** Get the full block magic string for files we write, it is the levels
   * appended together.  So "W2", "W", "R" becomes "W2-W-R" */
  static void
  blockLevelsToMagic(const std::vector<std::string>& blocks,
    std::string                                    & magic);

  /** Does the level of the file magic string match our block level?
   * Basically each part of magic string is a subclass identifier and
   * shows the existance of our data.  This allows dynamic file formats.
   * For any level, we have to match all levels up to this one.  For example,
   * "W2-R-Z" is the file level and we are subclass "W2-R-P".  Level 1 and 2
   * will match, but level 3 will not.  This means we can read data for level 1
   * and 2 only. */
  bool
  matchBlockLevel(size_t level);

  // ----------------------------------------------------------------------------

  /** Read our block from file if it exists at current location */
  virtual bool
  readBlock(FILE * fp);

  /** Write our block to file at current location */
  virtual bool
  writeBlock(FILE * fp);

  // ----------------------------------------------------------------------------
  // Query methods.  This allows writers such as the netcdf encoder to query
  // our information and store it for us

  /** Return the number of arrays of data we will store */
  size_t
  getArrayCount()
  {
    return (0);
  }

  /** Get the number of dimensions in our table data.  Typically for a 'single'
   * table this will be 1 */
  class TableInfo : public Data {
public:
    std::string name;
    size_t size;
    std::vector<std::string> columnNames;
    std::vector<std::string> columnUnits;

    // float, uchar, ushort
    std::vector<std::string> columnTypes;
  };

  virtual std::vector<TableInfo>
  getTableInfo()
  {
    std::vector<TableInfo> info;
    TableInfo i;
    i.name = "rows"; // First 'colllection' of data called rows
    i.size = 0;      // Subclasses should fill in with data size
    info.push_back(i);
    return (info);
  }

  /** The 'string' column type */
  virtual std::vector<std::string>
  getStringVector(const std::string& name)
  {
    return (std::vector<std::string>());
  }

  /** The 'float' column type */
  virtual std::vector<float>
  getFloatVector(const std::string& name)
  {
    return (std::vector<float>());
  }

  /** The 'uchar' column type */
  virtual std::vector<unsigned char>
  getUCharVector(const std::string& name)
  {
    return (std::vector<unsigned char>());
  }

  /** The 'ushort' column type */
  virtual std::vector<unsigned short>
  getUShortVector(const std::string& name)
  {
    return (std::vector<unsigned short>());
  }

protected:

  /** The last magic levels of a read block call.  Levels are from a string
   * in the file of the form W2-Level2-Level3, etc.  These represent the
   * subclass tree of the writing class, where each level is a 'block' of
   * data.  This allows dynamic expandsion of files. */
  std::vector<std::string> myLastFileBlockLevels;

  /** The last version number of a read block call */
  size_t myLastFileVersion;

  /** Our block level...which is level 1 and the first always.  Every subclass
   * should increase their count by 1 (depth of subclass tree.  Siblings have
   * same value) */
  static const size_t BLOCK_LEVEL;
};
}
