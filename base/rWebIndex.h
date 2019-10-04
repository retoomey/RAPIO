#pragma once

#include <rIndexType.h>
#include <rIOIndex.h>
#include <rURL.h>

namespace rapio {
/**
 * A index type which pulls from a changing web server.
 *
 */
class WebIndex : public IndexType, public IOListener {
public:

  WebIndex(){ }

  virtual
  ~WebIndex();

  // Factory methods ------------------------------------------

  virtual std::shared_ptr<IndexType>
  createIndexType(
    const URL                                    & location,
    std::vector<std::shared_ptr<IndexListener> > listeners,
    const TimeDuration                           & maximumHistory) override;

  static void
  introduceSelf();

  /** The URL here needs to be something like:
   *  http://venus.protect.nssl:8080/?source=KABR
   *  We'll add webIndex/WebIndexServlet etc. as required.
   */
  WebIndex(const URL                                  & website,
    const std::vector<std::shared_ptr<IndexListener> >& listeners,
    const TimeDuration                                & maximumHistory);

  // Index methods ------------------------------------------
  /** Handle realtime vs. archive mode stuff */
  virtual bool
  initialRead(bool realtime) override;

  // IOListener methods ---------------------------------------
  virtual bool
  handlePoll() override;

  /** Read all xml records from a remote changing URL */
  virtual bool
  readRemoteRecords();

  //  bool         isValid() const {
  //    return (myReadOK);
  //  }

private:

  URL myURL;

  long long myLastRead;
  long myLastReadNS;
  bool myReadOK;
  bool myFoundNew;
  URL indexDataPath;
  // long long myLastPollTime;
  // int myThrottleSecs;
};
}
