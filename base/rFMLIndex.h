#pragma once

#include <rFileIndex.h>
#include <rFAMWatcher.h>

namespace rapio {
/**
 * Index that is meant to be used with the FAM protocol of notification.
 * This relies on an index directory that is monitored by libfam.
 *
 * @author Robert Toomey
 */
class FMLIndex : public FileIndex {
public:

  /** Default constant for a FAM index */
  static const std::string FMLINDEX_FAM;

  /** Default constant for a polling FML index */
  static const std::string FMLINDEX_POLL;

  /** Get help for us */
  virtual std::string
  getHelpString(const std::string& fkey) override;

  /** Can we handle this protocol/path from -i?  Update allowed*/
  static bool
  canHandle(const URL& url, std::string& protocol, std::string& indexparams);

  // ---------------------------------------------------------------------
  // Factory

  /** Introduce ourselves to the index factories */
  static void
  introduceSelf();

  /** Create an individual FMLIndex to a particular location */
  virtual std::shared_ptr<IndexType>
  createIndexType(
    const std::string  & protocol,
    const std::string  & location,
    const TimeDuration & maximumHistory) override;

  /** Create a new empty FMLIndex, probably as main factory */
  FMLIndex(){ }

  /** Specialized constructor */
  FMLIndex(
    const std::string  & protocol,
    const URL          & index_directory_name,
    const TimeDuration & maximumHistory);

  /** Handle realtime vs. archive mode stuff */
  virtual bool
  initialRead(bool realtime, bool archive) override;

  /** Do we want to process this file? */
  virtual bool
  wantFile(const std::string& path) override;

  /** Handle a file */
  virtual void
  handleFile(const std::string& filename) override;

  /** Destroy a FMLIndex */
  virtual
  ~FMLIndex();

protected:

  /** The worker beast for turning a file into a record */
  bool
  fileToRecord(const std::string& filename, Record& rec);
}
;
}
