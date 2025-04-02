#pragma once

/** RAPIO API */
#include <RAPIO.h>

namespace rapio {
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
class MakeIndex : public rapio::RAPIOProgram {
public:

  /** Create */
  MakeIndex(){ };

  /** Declare all algorithm options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(rapio::RAPIOOptions& o) override;

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
  makeIndexDirWalker(const std::string& what, MakeIndex * owner, const std::string& suffix) : myWhat(what), myOwner(
      owner), mySuffix(suffix)
  { }

  /** Process a regular file */
  virtual Action
  processRegularFile(const std::string& filePath, const struct stat * info) override;

  /** Process a directory */
  virtual Action
  processDirectory(const std::string& dirPath, const struct stat * info) override;

protected:

  /** The action we take on a file */
  std::string myWhat;

  /** The roster that wants us */
  MakeIndex * myOwner;

  /** The suffix we are hunting such as ".cache" or ".mask" */
  std::string mySuffix;
};
}
