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

  /** Construct a BinaryTable */
  BinaryTable();

  /** Get the block level magic vector for this class.  Subclasses MUST override
   * to call their
   * superclass and then push back their level identifier. */
  virtual void
  getBlockLevels(std::vector<std::string>& levels) const;

  /** Return the version number of binary format.  Change on a
   * major change.  This will allow backward compatibility */
  size_t
  getVersion() const;

  /** Return the version number of last read file, or zero if not */
  size_t
  getLastFileVersion() const;

  /** Can we handle this version?  Default handles version
   * less than or equal to current version */
  virtual bool
  canHandleVersion(size_t version) const;

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
  matchBlockLevel(size_t level) const;

  // ----------------------------------------------------------------------------

  /** Read our block from file if it exists at current location */
  virtual bool
  readBlock(const std::string& path, FILE * fp);

  /** Write our block to file at current location */
  virtual bool
  writeBlock(FILE * fp);

  // ----------------------------------------------------------------------------
  // Query methods.  This allows writers such as the netcdf encoder to query
  // our information and store it for us

  /** Return the number of arrays of data we will store */
  size_t
  getArrayCount() const
  {
    return (0);
  }

  /** Get the number of dimensions in our table data.  Typically for a 'single'
   * table this will be 1 */
  class TableInfo {
public:
    std::string name;
    size_t size;
    std::vector<std::string> columnNames;
    std::vector<std::string> columnUnits;

    // float, uchar, ushort
    std::vector<std::string> columnTypes;
  };

  virtual std::vector<TableInfo>
  getTableInfo();

  /** The 'string' column type */
  virtual std::vector<std::string>
  getStringVector(const std::string& name);

  /** The 'float' column type */
  virtual std::vector<float>
  getFloatVector(const std::string& name);

  /** The 'uchar' column type */
  virtual std::vector<unsigned char>
  getUCharVector(const std::string& name);

  /** The 'ushort' column type */
  virtual std::vector<unsigned short>
  getUShortVector(const std::string& name);

  /** The 'char' column type */
  virtual std::vector<char>
  getCharVector(const std::string& name)
  {
    return std::vector<char>();
  }

  /** The 'short' column type */
  virtual std::vector<short>
  getShortVector(const std::string& name)
  {
    return std::vector<short>();
  }

  // ----------------------------------------------------------------------------
  // Generic column storage
  //
  // The base BinaryTable can hold a single table of columns.  This lets a
  // generic reader (such as the netcdf reader) build a table without needing
  // a specialized subclass.  Subclasses with their own layout simply override
  // the query methods above and ignore this storage.

  /** Add a 'string' column to the generic table */
  void
  addColumn(const std::string& name, const std::string& units,
    const std::vector<std::string>& data);

  /** Add a 'float' column to the generic table */
  void
  addColumn(const std::string& name, const std::string& units,
    const std::vector<float>& data);

  /** Add a 'ushort' column to the generic table */
  void
  addColumn(const std::string& name, const std::string& units,
    const std::vector<unsigned short>& data);

  /** Add a 'uchar' column to the generic table */
  void
  addColumn(const std::string& name, const std::string& units,
    const std::vector<unsigned char>& data);

  /** Do we have generic columns stored? */
  bool
  hasGenericColumns() const
  {
    return (!myColumns.empty());
  }

  virtual bool getUseMissingAsUnavailable(){ return false; }

protected:

  /** A single generic column (only the vector matching 'type' is used) */
  struct GenericColumn {
    std::string                 name;
    std::string                 units;
    std::string                 type;
    std::vector<std::string>    strings;
    std::vector<float>          floats;
    std::vector<unsigned short> ushorts;
    std::vector<unsigned char>  uchars;
  };

  /** Find a generic column by name, returning its index or -1 */
  int
  findColumn(const std::string& name) const;

  /** Generic columns, if any, stored in order */
  std::vector<GenericColumn> myColumns;

  /** Row count of the generic table */
  size_t myRowSize;


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
