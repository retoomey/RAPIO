#pragma once

#include <rRAPIOProgram.h>
#include <rDirWalker.h>
#include <memory>
#include <vector>

namespace rapio {
// -----------------------------------------------------------
// FIXME: Generalize into RAPIO, allow introduction of new parsers if
// we're trying to read other formats with more complicated time
// strings. For MRMS/HMRG I think the digit approach handles most
// cases. But it didn't handle the rap data format.  And I'm sure
// there are tons of others since nothing is standardized.
// I think some of Time.h could be moved into a TimeParser class.
//
/** Base interface for all time extraction strategies */
class TimeParser {
public:
  TimeParser(const std::string& name) : myName(name){ }

  virtual
  ~TimeParser() = default;

  /** Name of this time parser */
  const std::string&
  getName() const { return myName; }

  /** How many matched strings? */
  size_t
  getMatchCount() const { return myMatchCount; }


  /** Scan a filename string and attempt to populate the Time object. */
  virtual bool
  scan(const std::string& str, Time& set) = 0;
protected:
  size_t myMatchCount = 0;
  std::string myName;
};

/*
 * Yet another make index.
 *
 * This will scan directories and create a code_index.xml
 * metadata that can be used to feed algorithms in archive
 * mode.
 *
 * @author Valliappa Lakshman
 * @author Robert Toomey
 **/
class MakeIndex : public RAPIOProgram {
public:

  /** Create */
  MakeIndex(){ };

  /** Declare all algorithm options */
  virtual void
  declareOptions(RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(RAPIOOptions& o) override;

  /** Execute */
  virtual void
  execute() override;

protected:

  /** Root directory of our scanning for index creation */
  std::string myRoot;

  /** Output file name, typically code_index.xml */
  std::string myOutputFile;
};

class makeIndexDirWalker : public DirWalker
{
public:
  /** Create dir walker helper */
  makeIndexDirWalker(const std::string& what, MakeIndex * owner, const std::string& suffix);

  /** Process a regular file */
  virtual Action
  processRegularFile(const std::string& filePath, const struct stat * info) override;

  /** Process a directory */
  virtual Action
  processDirectory(const std::string& dirPath, const struct stat * info) override;

  /** Print final stats */
  void
  printStats();

protected:

  /** The action we take on a file */
  std::string myWhat;

  /** The roster that wants us */
  MakeIndex * myOwner;

  /** The suffix we are hunting such as ".cache" or ".mask" */
  std::string mySuffix;

  /** Our chain of cached time parsers */
  std::vector<std::shared_ptr<TimeParser> > myParsers;
};
}
