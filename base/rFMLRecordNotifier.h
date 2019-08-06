#pragma once

#include <rRecordNotifier.h>

namespace rapio {
/**
 * Subclasses are database-specific implementations.
 */
class FMLRecordNotifier : public RecordNotifier {
public:

  /** Introduce a record notifier that writes .fml files */
  static void
  introduceSelf();

  virtual
  ~FMLRecordNotifier();

  virtual void
  writeRecord(const Record& rec) override;

  virtual void
  setURL(URL aURL, URL datalocation) override;

  /** Make the output fml directories needed */
  bool
  makeDirectories();

protected:

  std::string myOutputDir, myTempDir, myIndexPath;
}
;
}
