#include <rRecordQueue.h>

#include <rRAPIOAlgorithm.h>
#include <rError.h>

#include <queue>

using namespace rapio;
using namespace std;

// In theory we could wrap
long long RecordQueue::pushedRecords = 0;
long long RecordQueue::poppedRecords = 0;

RecordQueue::RecordQueue(
  RAPIOAlgorithm * alg
) : EventHandler("RecordQueue") // Run me as fast as you can
{
  myAlg = alg; // Only for archive stop...hummm
}

void
RecordQueue::addRecord(Record& record)
{
  myQueue.push(record);
  pushedRecords++;
  // Record pushed, notify ready for action
  setReady();
}

void
RecordQueue::addRecords(std::vector<Record>& records)
{
  for (auto&r:records) {
    // This should auto sort records..
    myQueue.push(r); // copy or move?  We should switch to pointers I think...
    pushedRecords++;
  }
  // Have more available to process
  if (!myQueue.empty()) {
    setReady();
  }
}

void
RecordQueue::action()
{
  // Process one record if there...
  if (!myQueue.empty()) {
    fLogInfo("Record queue size is {}", myQueue.size());
    Record r = myQueue.top();
    myQueue.pop();
    poppedRecords++;
    myAlg->handleRecordEvent(r);
  }

  // If queue empty (possibly post processing one, fire end event)
  if (myQueue.empty()) {
    myAlg->handleEndDatasetEvent();
  } else {
    // ...otherwise we want to fire again
    setReady();
  }
}
