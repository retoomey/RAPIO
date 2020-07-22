#pragma once

#include <rIndexType.h>
#include <rFAMWatcher.h>

namespace rapio {
/**
 * Index that is meant to be used with the FAM protocol of notification.
 * This relies on an index directory that is monitored by libfam.
 *
 * @author Robert Toomey
 */
class FileIndex : public IndexType {
public:

  /** Default constant for a FAM index */
  static const std::string FileINDEX_FAM;

  /** Default constant for a polling File index */
  static const std::string FileINDEX_POLL;

  // ---------------------------------------------------------------------
  // Factory

  /** Introduce ourselves to the index factories */
  static void
  introduceSelf();

  /** Create an individual FileIndex to a particular location */
  virtual std::shared_ptr<IndexType>
  createIndexType(
    const std::string  & protocol,
    const std::string  & location,
    const TimeDuration & maximumHistory) override;

  /** Create a new empty FileIndex, probably as main factory */
  FileIndex(){ }

  /** Specialized constructor */
  FileIndex(
    const std::string  & protocol,
    const URL          & index_directory_name,
    const TimeDuration & maximumHistory);

  /** Handle realtime vs. archive mode stuff */
  virtual bool
  initialRead(bool readtime, bool archive) override;

  /** Handle a new file from a watcher.  We're allowed to do work here. */
  virtual void
  handleNewEvent(WatchEvent * w) override;

  /** Handle a file */
  virtual void
  handleFile(const std::string& filename);

  /** Handle a new directory */
  virtual void
  handleNewDirectory(const std::string& dirname);

  /** Handle unmount */
  virtual void
  handleUnmount(const std::string& dirname, bool willReconnect);

  /** Destroy a FileIndex */
  virtual
  ~FileIndex();

private:

  /** Protocol used for polling */
  std::string myProtocol;

  /** URL of the watched location */
  URL myURL;

  /** Resolved index path to the polled location */
  std::string myIndexPath;
}
;
}
