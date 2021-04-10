#pragma once

#include <rIndexType.h>
#include <rIOIndex.h>
#include <rURL.h>

namespace rapio {
/**
 * A index type which pulls from a changing web server.
 * @author Robert Toomey
 */
class WebIndex : public IndexType {
public:

  /** Default constant for a FAM index */
  static const std::string WEBINDEX;

  WebIndex(){ }

  virtual
  ~WebIndex();

  /** Get help for us */
  virtual std::string
  getHelpString(const std::string& fkey) override;

  /** Can we handle this protocol/path from -i?  Update allowed*/
  static bool
  canHandle(const URL& url, std::string& protocol, std::string& indexparams);

  // Factory methods ------------------------------------------

  virtual std::shared_ptr<IndexType>
  createIndexType(
    const std::string  & protocol,
    const std::string  & location,
    const TimeDuration & maximumHistory) override;

  static void
  introduceSelf();

  /** The URL here needs to be something like:
   *  http://venus.protect.nssl:8080/?source=KABR
   *  We'll add webIndex/WebIndexServlet etc. as required.
   */
  WebIndex(const URL   & website,
    const TimeDuration & maximumHistory);

  // Index methods ------------------------------------------
  /** Handle realtime vs. archive mode stuff */
  virtual bool
  initialRead(bool realtime, bool archive) override;

  // IOListener methods ---------------------------------------
  virtual bool
  handlePoll() override;

  /** Read all xml records from a remote changing URL */
  virtual bool
  readRemoteRecords();

private:

  /** A time marker for web index records so we don't reread older records */
  long long myLastRead;

  /** A time marker (extra nanoseconds) for web index records so we don't reread older records */
  long myLastReadNS;

  /** The base URL for pulling web records */
  URL myURL;

  /** The URL for pulling web records */
  URL indexDataPath;
};
}
