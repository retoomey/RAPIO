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
class FMLIndex : public IndexType {
public:

  /** Default constant for a FAM index */
  static const std::string FMLINDEX_FAM;

  /** Default constant for a polling FML index */
  static const std::string FMLINDEX_POLL;

  // ---------------------------------------------------------------------
  // Factory

  /** Introduce ourselves to the index factories */
  static void
  introduceSelf();

  /** Create an individual FMLIndex to a particular location */
  virtual std::shared_ptr<IndexType>
  createIndexType(
    const std::string                            & protocol,
    const std::string                            & location,
    std::vector<std::shared_ptr<IndexListener> > listeners,
    const TimeDuration                           & maximumHistory) override;

  /** Create a new empty FMLIndex, probably as main factory */
  FMLIndex(){ }

  /** Specialized constructor */
  FMLIndex(
    const std::string                                 & protocol,
    const URL                                         & index_directory_name,
    const std::vector<std::shared_ptr<IndexListener> >& listeners,
    const TimeDuration                                & maximumHistory);

  /** Handle realtime vs. archive mode stuff */
  virtual bool
  initialRead(bool realtime) override;

  /** Handle a new file from a watcher.  We're allowed to do work here. */
  virtual void
  handleNewFile(const std::string& filename) override;

  /** Handle our directory being unmounted. */
  virtual void
  handleUnmount(const std::string& dir) override;

  /** Destroy a FMLIndex */
  virtual
  ~FMLIndex();

protected:

  /** Do we want to process this file? */
  bool
  wantFile(const std::string& path);

  /** The worker beast for turning a file into a record */
  bool
  fileToRecord(const std::string& filename, Record& rec);

private:

  /** Protocol used for polling the fml */
  std::string myProtocol;

  /** URL of the watched location */
  URL myURL;

  /** Resolved index path to the polled location */
  std::string myIndexPath;
}
;
}
