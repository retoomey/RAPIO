#pragma once

#include <rEventTimer.h>
#include <rRAPIOAlgorithm.h>
#include <rError.h>

#include <vector>
#include <algorithm>
#include <queue>

namespace rapio {
/** Sort records for queue.  Usually this is in decreasing time order */
class RecordQueueSort
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
class RecordQueue : public EventHandler
{
public:
  RecordQueue(
    RAPIOAlgorithm * alg
  );

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

  /** Simple counter of pushed records */
  static long long pushedRecords;

  /** Simple counter of popped records */
  static long long poppedRecords;

  /** Low level access the queue if needed */
  std::priority_queue<Record, std::vector<Record>, RecordQueueSort>&
  getQueue(){ return myQueue; }

protected:

  /** The algorithm we send records to */
  RAPIOAlgorithm * myAlg;

  /** Records.  Stored in time order by record operator < */
  std::priority_queue<Record, std::vector<Record>, RecordQueueSort> myQueue;
};
}
