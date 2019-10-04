#pragma once

#include <rIO.h>

#include <rRecord.h>
#include <rURL.h>

#include <vector>

namespace rapio {
/** Abstract base class of RecordNotifiers.  */
class RecordNotifier : public IO {
public:

  // Instance methods -------------------------------------

  /** Notify about this record. */
  virtual void
  writeRecord(const Record& rec) = 0;

  /** The default implementation loops through the records,
   *  passing each to writeRecord(). */
  virtual void
  writeRecords(const std::vector<Record>& rec);

  /** Get the URL output location of this record notifier */
  virtual URL
  getURL()
  {
    return (myURL);
  }

  /** Set the URL output location of this record notifier */
  virtual void
  setURL(URL aURL, URL datalocation)
  {
    myURL = aURL;
  }

  /** Destroy a record notifier */
  virtual ~RecordNotifier(){ }

  // Factory methods --------------------------------------

  /** Introduce base notifier classes on startup */
  static void
  introduceSelf();

  /** Introduce subclass into factories */
  static void
  introduce(const std::string       & protocol,
    std::shared_ptr<RecordNotifier> factory);

  /** returns a notifier that will notify data. */
  static std::shared_ptr<RecordNotifier>
  getNotifier(
    const URL        & notifyLoc, // Suggested location for notification out
    const URL        & dataLoc,   // Data location can help with default settings
    const std::string& type = "fml");

  /** returns the default notification location in a directory.
   *  For example:  code_index.lb or code_index.fam depending on protocol
   */
  static std::string
  getDefaultBaseName();

protected:

  URL myURL;
}
;
}
