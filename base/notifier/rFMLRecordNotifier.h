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

  /** Create uninitialized FML record notifier */
  FMLRecordNotifier(){ }

  /** Destroy FML record notifier */
  virtual
  ~FMLRecordNotifier();

  /** Get help for us */
  static std::string
  getHelpString(const std::string& fkey);

  /** Calculate output directory and index location for FML record/message from parameters */
  void
  getOutputFolder(std::map<std::string, std::string>& outputParams,
    std::string& outputDir, std::string& indexLocation);

  /** Notify about this message. */
  virtual void
  writeMessage(std::map<std::string, std::string>& outputParams, const Message& m) override;

  /** Notify for a single record */
  virtual void
  writeRecord(std::map<std::string, std::string>& outputParams, const Record& rec) override;

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
