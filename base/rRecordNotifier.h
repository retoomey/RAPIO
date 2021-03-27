#pragma once

#include <rIO.h>

#include <rRecord.h>

namespace rapio {
/** Record notifier types registered */
class RecordNotifierType : public IO {
public:

  /** Create a record notifier */
  RecordNotifierType(){ }

  /** Destory a record notifier */
  ~RecordNotifierType(){ }

  /** Notify about this record. */
  virtual void
  writeRecord(const std::string& outputinfo, const Record& rec) = 0;

  /** The default implementation loops through the records,
   *  passing each to writeRecord(). */
  virtual void
  writeRecords(const std::string& outputinfo, const std::vector<Record>& rec);

  /** Set the initial params this record notifier */
  virtual void
  initialize(const std::string& params) = 0;
};

/** Factory container/helper for RecordNotifierType */
class RecordNotifier : public IO {
public:

  // Factory methods --------------------------------------

  /** Introduce base notifier classes on startup */
  static void
  introduceSelf();

  /** Introduce base notifier classes help on startup.  Note that
   * introduceSelf may have not been called yet. */
  static std::string
  introduceHelp();

  /** Process definition string for RAPIOAlgorithm.  Return
   * vector of created notifiers */
  static bool
  createNotifiers(const std::string                  & nstring,
    std::vector<std::shared_ptr<RecordNotifierType> >& n);

  /** Create a new notifier for notifying new data. */
  static std::shared_ptr<RecordNotifierType>
  createNotifier(
    const std::string & type,
    const std::string & params);
}
;
}
