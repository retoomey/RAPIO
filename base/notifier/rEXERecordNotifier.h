#pragma once

#include <rRecordNotifier.h>

namespace rapio {
/**
 * Run an external program, passing record information
 * to it.
 */
class EXERecordNotifier : public RecordNotifierType {
public:

  /** Introduce a record notifier that writes .fml files */
  static void
  introduceSelf();

  /** Factory creation */
  static std::shared_ptr<RecordNotifierType>
  create();

  /** Create uninitialized FML record notifier */
  EXERecordNotifier(){ }

  /** Destroy FML record notifier */
  virtual
  ~EXERecordNotifier();

  /** Get help for us */
  static std::string
  getHelpString(const std::string& fkey);

  /** Notify about this message. */
  virtual void
  writeMessage(std::map<std::string, std::string>& outputParams, const Message& m) override;

  /** Notify for a single record */
  virtual void
  writeRecord(std::map<std::string, std::string>& outputParams, const Record& rec) override;

  /** Handle params for notifier */
  virtual void
  initialize(const std::string& params) override;

protected:

  /** Executable for notification */
  std::string myExe;
}
;
}
