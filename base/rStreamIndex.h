#pragma once

#include <rIndexType.h>
#include <rFAMWatcher.h>
#include <rStrings.h>

namespace rapio {
/**
 * Index that inputs FML metadata from an external binary or script
 * Example: exe="feedme -f TEXT" for local ldm ingest
 *
 * @author Robert Toomey
 */
class StreamIndex : public IndexType {
public:

  /** Default constant for a STREAM index */
  static const std::string STREAMINDEX;

  /** Get help for us */
  virtual std::string
  getHelpString() override;
  // ---------------------------------------------------------------------
  // Factory

  /** Introduce ourselves to the index factories */
  static void
  introduceSelf();

  /** Create an individual StreamIndex to a particular location */
  virtual std::shared_ptr<IndexType>
  createIndexType(
    const std::string  & protocol,
    const std::string  & location,
    const TimeDuration & maximumHistory) override;

  /** Create a new empty StreamIndex, probably as main factory */
  StreamIndex() : myItemStart(""), myItemEnd(""){ }

  /** Specialized constructor */
  StreamIndex(
    const std::string  & protocol,
    const std::string  & indexparams,
    const TimeDuration & maximumHistory);

  /** Handle realtime vs. archive mode stuff */
  virtual bool
  initialRead(bool realtime, bool archive) override;

  /** Handle a new event from a watcher.  We're allowed to do work here. */
  virtual void
  handleNewEvent(WatchEvent * event) override;

  /** Destroy a StreamIndex */
  virtual
  ~StreamIndex();

protected:
  /** Command arguments/params for execution */
  std::string myParams;

  /** Current input stream token gathering */
  std::vector<char> myLineCout;

  /** Current DFA state hunting for tags */
  size_t myDFAState;

  /** My start tag */
  DFAWord myItemStart;

  /** My end tag */
  DFAWord myItemEnd;

private:
}
;
}
