#pragma once

#include <rIO.h>

#include <rRecord.h>
#include <rURL.h>

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
  writeRecord(const Record& rec, const std::string& file) = 0;

  /** The default implementation loops through the records,
   *  passing each to writeRecord(). */
  virtual void
  writeRecords(const std::vector<Record>& rec, const std::vector<std::string>& files);

  /** Set the initial params this record notifier */
  virtual void
  initialize(const std::string& params, const std::string& outputdir) = 0;
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
  static void
  introduceHelp(std::string& help);

  /** Process definition string for RAPIOAlgorithm.  Return
   * vector of created notifiers */
  static bool
  createNotifiers(const std::string                  & nstring,
    const std::string                                & outputdir,
    std::vector<std::shared_ptr<RecordNotifierType> >& n);

  /** Returns a notifier that will notify data. */
  static std::shared_ptr<RecordNotifierType>
  getNotifier(
    const std::string & params,
    const std::string & outputdir,
    const std::string & type);
}
;
}
