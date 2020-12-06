#pragma once

#include <rRecordNotifier.h>

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
  writeRecord(const Record& rec, const std::string& file) override;

  /** Handle params for notifier */
  virtual void
  initialize(const std::string& params, const std::string& outputdir) override;

  /** Make the output fml directories needed */
  bool
  makeDirectories();

protected:

  /** Output directory for the FML notification */
  std::string myOutputDir;

  /** Temporary directory for async/move writing */
  std::string myTempDir;

  /** Full index path for substitution */
  std::string myIndexPath;

  /** URL to the output location for fml files */
  URL myURL;
}
;
}
