#include <rRecordQueue.h>

#include <rRAPIOAlgorithm.h>
#include <rError.h>

#include <queue>

using namespace rapio;
using namespace std;

RecordQueue::RecordQueue(
  RAPIOAlgorithm * alg
) : EventTimer(0, "RecordQueue") // Run me as fast as you can
{
  myAlg = alg; // Only for archive stop...hummm
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
RecordQueue::action()
{
  if (myQueue.empty()) {
    myAlg->handleEndDatasetEvent();
  } else {
    Record r = myQueue.top();
    myQueue.pop();
    myAlg->handleRecordEvent(r);
  }
}
