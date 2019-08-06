#pragma once

#include <rIO.h>
#include <rEventTimer.h>
#include <rRAPIOAlgorithm.h>
#include <rError.h>

#include <vector>
#include <algorithm>

namespace rapio {
/** Sort records for queue.  Usually this is in decreasing time order */
class RecordQueueSort : public Data
{
public:
  bool
  operator () (const Record& lhs, const Record& rhs) const
  {
    return rhs < lhs; // reverse time for queue
  }
};

/** Record queue holds Records that will be sent to be processed when able.
 * @author Robert Toomey
 */
class RecordQueue : public EventTimer
{
public:
  RecordQueue(
    RAPIOAlgorithm * alg,
    bool             realtime
  );

  RAPIOAlgorithm * myAlg;
  bool myRealtime;

  std::vector<std::shared_ptr<IndexType> > myConnectedIndexes;
  size_t indexsize;

  /** Records.  Stored in time order by record operator < */
  std::priority_queue<Record, std::vector<Record>, RecordQueueSort> myQueue;

  /** Run full speed.  Not sure we need to override this... */
  virtual double
  readyInMS(std::chrono::time_point<std::chrono::high_resolution_clock> at) override { return 0.0; }

  /** Set connected indexes for event queue.  Needed before action */
  void
  setConnectedIndexes(std::vector<std::shared_ptr<IndexType> > c);

  /** Add given record to queue */
  void
  addRecord(Record& record);

  /** Add given records to queue */
  void
  addRecords(std::vector<Record>& records);

  /** Size of our queue */
  size_t
  size(){ return myQueue.size(); }

  /** Fired action.  Usually process a record from queue */
  virtual void
  action() override;
};
}
