#pragma once

#include <rIndexType.h>
#include <rFAMWatcher.h>

namespace rapio {
/**
 * Index that is meant to be used with the FAM protocol of notification.
 * This relies on an index directory that is monitored by libfam.
 *
 */
class FMLIndex : public IndexType {
public:

  // Factory
  // ---------------------------------------------------------------------

  FMLIndex(){ }

  /** Create a FMLIndex */
  virtual std::shared_ptr<IndexType>
  createIndexType(
    const URL                                    & location,
    std::vector<std::shared_ptr<IndexListener> > listeners,
    const TimeDuration                           & maximumHistory) override;

  FMLIndex(const URL                                  & index_directory_name,
    const std::vector<std::shared_ptr<IndexListener> >& listeners,
    const TimeDuration                                & maximumHistory);

  virtual
  ~FMLIndex();

  /** Do we want to process this file? */
  bool
  wantFile(const std::string& path);

  /** Handle a new file from a watcher.  We're allowed to do work here. */
  virtual void
  handleNewFile(const std::string& filename) override;

  /** Handle our directory being unmounted. */
  virtual void
  handleUnmount(const std::string& dir) override;

  /** The worker beast for turning a file into a record */
  bool
  fileToRecord(const std::string& filename, Record& rec);

  /** Was thie FAM index sucessfully set up? */
  bool
  isValid() const
  {
    return (is_valid);
  }

  void
  initialDirectory(const std::string& dirname);

  /** Handle realtime vs. archive mode stuff */
  virtual bool
  initialRead(bool realtime) override;

  static void
  introduceSelf();

private:

  std::string indexPath;
  bool is_valid;

  // not implemented
  FMLIndex(const FMLIndex&);
  FMLIndex&
  operator = (const FMLIndex&);
}
;
}
