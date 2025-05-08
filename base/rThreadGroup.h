#pragma once

#include <rUtility.h>
#include <rError.h>
#include <rDataType.h>
#include <rBOOST.h>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>
#include <atomic>
#include <future>

namespace rapio {
/** What we do with a worker thread?  Subclass to do more */
class ThreadTask : public Utility {
public:

  /** Create a thread task with a future */
  ThreadTask() : myFuture(myPromise.get_future()){ }

  /** Do our job.  Override to do your stuff. */
  virtual void
  execute()
  {
    LogInfo("--->Thread task execute that does nothing. You should be subclassing ThreadTask.\n");
    // Any subclass MUST call markDone when finished, or calling thread will
    // hang waiting on you forever.
    markDone();
  };

  /** Mark the task as complete (call this at end of execute) */
  void
  markDone()
  {
    myPromise.set_value();
  }

  /** Block until this task is finished */
  void
  waitUntilDone()
  {
    myFuture.wait();
  }

  /** Returns true if task has finished */
  bool
  isFinished() const
  {
    return myFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
  }

protected:
  std::promise<void> myPromise;
  std::future<void> myFuture;
};

/** Write the standard output thread task.
 * Oh boy.  Can of worms I gotta go back and thread check the entire call tree here.
 * This is an experiment, I expect sync issues until I harden the code. */
class WriteOutputThreadTask : public ThreadTask {
public:
  /** Create a write output thread task. */
  WriteOutputThreadTask(const std::string& key,
    std::shared_ptr<DataType>            outputData,
    std::map<std::string, std::string>   & outputParams);

  /** Do our job.  Override to do your stuff. */
  virtual void
  execute();

protected:

  /** The product key used to write the DataType */
  std::string myKey;

  /** The map of extra output params for the DataType */
  std::map<std::string, std::string> myOutputParams;
};

/**
 * A utility for running thread pool of queued tasks
 * Note currently keeping each subclass group unique,
 * though possibly you could share a global thread pool
 * with generic task ability.
 *
 * Using boost::thread_group, but maybe use tbb later.
 *
 * @author Robert Toomey
 */
class ThreadGroup : public Utility {
public:
  /** Create a thread pool with given number of workers, and a max number of tasks it can queue. */
  ThreadGroup(size_t maxWorkers, size_t maxQueueSize);

  /** Add a ThreadTask to the task queue.  @return false is queue is full.  Retry in a loop typically */
  bool
  enqueueThreadTask(std::shared_ptr<ThreadTask> task);

  /** On destruction, clean up threads */
  virtual
  ~ThreadGroup();

private:

  /** Main worker thread responsible for popping tasks from queue and executing */
  void
  workerThread();

protected:

  /** Maximum number of helper thread tasks we create */
  size_t myMaxWorkers;

  /** Maximum number of tasks we can buffer */
  size_t myMaxQueueSize;

  /** Queue of tasks needing to be done */
  std::queue<std::shared_ptr<ThreadTask> > myTaskQueue;

  /** Stop all threads when finalizing/exiting */
  bool myStop;

  /** Lock for myStop */
  std::mutex myStopMutex;

  /** Lock for myTaskQueue */
  std::mutex myTaskQueueMutex;

  /** Thread notification condition */
  std::condition_variable myCondition;

  /** Boost thread group */
  boost::thread_group myWorkers;
};
}
