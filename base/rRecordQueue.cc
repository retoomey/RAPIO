#include <rRecordQueue.h>

#include <rRAPIOAlgorithm.h>
#include <rError.h>

#include <queue>

using namespace rapio;
using namespace std;

RecordQueue::RecordQueue(
  RAPIOAlgorithm * alg,
  bool             realtime
) : EventTimer(0, "RecordQueue") // Run me as fast as you can
{
  myAlg      = alg; // Only for archive stop...hummm
  myRealtime = realtime;
}

void
RecordQueue::addRecord(Record& record)
{
  myQueue.push(record);
}

void
RecordQueue::addRecords(std::vector<Record>& records)
{
  for (auto&r:records) {
    // This should auto sort records..
    myQueue.push(r); // copy or move?  We should switch to pointers I think...
  }
}

void
RecordQueue::setConnectedIndexes(
  std::vector<std::shared_ptr<IndexType> > c)
{
  myConnectedIndexes = c;
  indexsize = myConnectedIndexes.size();
}

void
RecordQueue::action()
{
  if (myQueue.empty()) {
    if (!myRealtime) {
      myAlg->handleEndDatasetEvent(); // Archive finished, let algorithms handle it
    }
  } else {
    Record r = myQueue.top();
    myQueue.pop();
    const size_t c = r.getIndexNumber();
    if (c < indexsize) {
      myConnectedIndexes[c]->processRecord(r); // Do the real work...
    } else {
      LogSevere(
        "Code error: Skipping record with bad index count..wasn't set properly.\n");
    }
  }
}
