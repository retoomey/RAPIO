#pragma once

#include <rRecord.h>

namespace rapio {
/** Record notifier types registered
 * @ingroup rapio_IO
 * */
class RecordNotifierType {
public:

  /** Create a record notifier */
  RecordNotifierType(){ }

  /** Destory a record notifier */
  ~RecordNotifierType(){ }

  /** Notify about this message. */
  virtual void
  writeMessage(std::map<std::string, std::string>& outputParams, const Message& m) = 0;

  /** Notify about this record. */
  virtual void
  writeRecord(std::map<std::string, std::string>& outputParams, const Record& rec) = 0;

  /** The default implementation loops through the records,
   *  passing each to writeRecord(). */
  virtual void
  writeRecords(std::map<std::string, std::string>& outputParams, const std::vector<Record>& rec);

  /** Set the initial params this record notifier */
  virtual void
  initialize(const std::string& params) = 0;
};

/** Factory container/helper for RecordNotifierType
 * @ingroup rapio_io
 * */
class RecordNotifier {
public:

  // Factory methods --------------------------------------

  /** Introduce base notifier classes on startup */
  static void
  introduceSelf();

  /** Introduce base notifier classes help on startup.  Note that
   * introduceSelf may have not been called yet. */
  static std::string
  introduceHelp();

  /** Return vector of created notifiers */
  static std::vector<std::shared_ptr<RecordNotifierType> >
  createNotifiers();

  /** Create a new notifier for notifying new data. */
  static std::shared_ptr<RecordNotifierType>
  createNotifier(
    const std::string & type,
    const std::string & params);
};

/** Create class to create a new instance of this notifier,
 * allowing us to use the Factory class normally
 * @ingroup rapio_io
 * @brief Factory to create a new instance of a RecordNotifierType */
class RecordNotifierCreator {
public:
  virtual
  ~RecordNotifierCreator() = default;
  /** Return a new instance */
  virtual std::shared_ptr<RecordNotifierType>
  create(const std::string& params) = 0;
};
}
