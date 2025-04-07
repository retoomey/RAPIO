#include "rThreadGroup.h"

#include "rError.h"

using namespace rapio;

WriteOutputThreadTask::WriteOutputThreadTask(const std::string& key, std::shared_ptr<DataType> outputData,
  std::map<std::string, std::string>& outputParams) : myKey(key), myOutputParams(outputParams)
{
  // FIXME: Will need the key, params, DataType
  // outputData
}

void
WriteOutputThreadTask::execute()
{
  // Will need the app pointer.
  //  app->writeOutputProduct(myProductKey, myDataType, myExtraParams);
  markDone();
};

ThreadGroup::ThreadGroup(size_t maxWorkers, size_t maxQueueSize)
  : myMaxWorkers(maxWorkers), myMaxQueueSize(maxQueueSize), myStop(false)
{
  // Start worker threads.  FIXME: I guess this could fail, so maybe a seperate method?
  for (size_t i = 0; i < myMaxWorkers; ++i) {
    myWorkers.create_thread(std::bind(&ThreadGroup::workerThread, this));
  }
}

bool
ThreadGroup::enqueueThreadTask(std::shared_ptr<ThreadTask> task)
{
  {
    std::unique_lock<std::mutex> lock(myTaskQueueMutex);
    if (myTaskQueue.size() >= myMaxQueueSize) {
      // Handle queue full situation, maybe wait or drop the item
      // LogInfo("---Queue is full. Not adding task." << "\n");
      return false;
    }
    myTaskQueue.push(task);
    // static std::atomic<size_t> counter(0);
    // LogInfo("---Data enqueued. Queue size: " << myTaskQueue.size() << " ID#: "<< ++counter << "\n");
    // if (counter >= 2000){ counter = 0; }
  }
  /** Tell one of our workers to pop the queue */
  myCondition.notify_one();
  return true;
}

/** Main worker thread responsible for popping tasks from queue and executing */
void
ThreadGroup::workerThread()
{
  while (true) {
    // This should only be calling once on start and once after a job pretty much
    std::shared_ptr<ThreadTask> task;

    {
      std::unique_lock<std::mutex> lock(myTaskQueueMutex);

      myCondition.wait(lock, [this] {
        return myStop || !myTaskQueue.empty();
      });
      if (myStop && myTaskQueue.empty()) {
        return;
      }
      task = myTaskQueue.front();
      myTaskQueue.pop();
      // LogInfo("---Data dequeued. Queue size: " << myTaskQueue.size() << "\n");
    }

    task->execute();

    // static std::atomic<size_t> counter(0);
    // LogInfo("---Data processed." << ++counter << "\n");
  }
}

ThreadGroup::~ThreadGroup()
{
  {
    // Lock the mutex to safely modify shared data
    std::unique_lock<std::mutex> lock(myStopMutex);
    // Set the stop flag to true to signal threads to stop
    myStop = true;
  } // End of inner scope: mutex is automatically unlocked when leaving this scope
  LogInfo("Shutting down a thread group " << (void *) (this) << "\n");
  // Notify all threads that they should wake up
  myCondition.notify_all();
  // Join all worker threads to wait for them to finish
  myWorkers.join_all();
}
