#pragma once

#include <rRecordNotifier.h>

#include <map>

namespace rapio {
/**
 * Subclasses are database-specific implementations.
 */
class FMLRecordNotifier : public RecordNotifierType {
public:

  /** Introduce a record notifier that writes .fml files */
  static void
  introduceSelf();

  /** Factory creation */
  static std::shared_ptr<RecordNotifierType>
  create();

  /** Create uninitialized FML record notifier */
  FMLRecordNotifier(){ }

  /** Destroy FML record notifier */
  virtual
  ~FMLRecordNotifier();

  /** Get help for us */
  static std::string
  getHelpString(const std::string& fkey);

  /** Notify for a single record */
  virtual void
  writeRecord(const std::string& outputinfo, const Record& rec) override;

  /** Handle params for notifier */
  virtual void
  initialize(const std::string& params) override;

  /** Make the output fml directories needed */
  bool
  makeDirectories(const std::string& outdir, const std::string& tempdir);

protected:

  /** Overridden output directory for the FML notification */
  std::string myOutputDir;

  /** Output directory to indexLocation macro mapping */
  std::map<std::string, std::string> myIndexPaths;
}
;
}
