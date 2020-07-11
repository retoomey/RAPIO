#include "rEXEWatcher.h"

#include "rError.h" // for LogInfo()
#include "rOS.h"
#include "rStrings.h"

#include <queue>

#include <spawn.h>    // posix_spawn
#include <poll.h>     // poll
#include <sys/wait.h> // waitpid
#include <unistd.h>   // env

using namespace rapio;

void
EXEWatcher::EXEInfo::createEvents(WatcherType * w)
{
  if (!myConnected) {
    // Reconnect on fail? For now I'll just warn
    LogSevere(">>>Not connected to exe watched process\n");
    return;
  }

  // FIXME: Could make configurable if needed
  const size_t POLL_BUFFER_SIZE = 1024;
  const int TIMEOUT    = 100;
  const size_t MAXPASS = 5;

  std::vector<char> buffer;
  buffer.resize(POLL_BUFFER_SIZE);

  // Could store this in class maybe
  std::vector<pollfd> plist = { { myCoutPipe[0], POLLIN }, { myCerrPipe[0], POLLIN } };

  size_t counter = 0;
  bool ended     = false;

  WatchEvent aCoutEvent(myListener, "pipe", "");
  // WatchEvent aCerrEvent(myListener); // want or not? Ignoring fix pass

  for (int rval; (rval = poll(&plist[0], plist.size(), TIMEOUT)) > 0;) {
    // Check for various pipes
    if (plist[0].revents & POLLIN) {
      int bytes_read = read(myCoutPipe[0], &buffer[0], POLL_BUFFER_SIZE);

      // reserve memcpy might be quickest here, just making it work for moment
      for (int z = 0; z < bytes_read; z++) {
        aCoutEvent.myBuffer.push_back(buffer[z]);
      }
    } else if (plist[1].revents & POLLIN) {
      // Clear it out
      read(myCerrPipe[0], &buffer[0], POLL_BUFFER_SIZE);
    } else {
      // Process ended
      ended = true;
      break; // nothing left to read
    }

    // The normal loop only breaks if the process has not generated output
    // in the TIMEOUT period.  However if it's spamming, we'd never break,
    // so we process at most MAXPASS messages before yielding
    counter++;
    if (counter > MAXPASS) {
      break;
    }
  } // END FOR

  // Add new event if we got data
  if (aCoutEvent.myBuffer.size() > 0) {
    w->addEvent(aCoutEvent);
  }

  //  kill(myPid, SIGKILL); // Kill the spawned thing..

  // This always blocks...now if we fell through it died, so ok...
  if (ended) {
    LogInfo("Watched process external termination.\n");
    // This is blocking for whole program.  Are we ok here?
    int exit_code;
    waitpid(myPid, &exit_code, 0);
    LogInfo("--->Exit code: " << exit_code << "\n");
    // Make sure our pipes closed
    posix_spawn_file_actions_destroy(&myFA);
    close(myCoutPipe[0]);
    close(myCerrPipe[0]);
    myConnected = false;
  }
} // EXEWatcher::EXEInfo::createEvents

void
EXEWatcher::getEvents()
{
  // Watches can create events if the event creation process
  // is independent of other event processes.
  for (auto& w:myWatches) {
    w->createEvents(this);
  }
} // EXEWatcher::getEvents

bool
EXEWatcher::EXEInfo::connect()
{
  // https://unix.stackexchange.com/questions/252901/get-output-of-posix-spawn
  // https://stackoverflow.com/questions/13893085/posix-spawnp-and-piping-child-output-to-a-string
  // Popen not flexible enough, we some crazy asynchronous stuff
  // I'm avoiding threads at moment by event loop polling, threading 'might' be more efficient here
  // but any master coder knows this brings in mutexes and extra fun stuff.

  // Using % to separate arguments since ", ' and / are already used heavily for grouping
  std::vector<std::string> argsin;
  Strings::splitWithoutEnds(myParams, '%', &argsin);

  // Create our own pipes
  if (pipe(myCoutPipe) || pipe(myCerrPipe)) {
    LogSevere("Failed to create pipes for processes\n");
    return false;
  }

  int ret;
  ret = posix_spawn_file_actions_init(&myFA);

  // Make child send stuff to us
  // close is kinda like listening, open is the push >> of the child..kinda..uggggh
  ret = posix_spawn_file_actions_addclose(&myFA, myCoutPipe[0]);
  ret = posix_spawn_file_actions_addclose(&myFA, myCerrPipe[0]);
  ret = posix_spawn_file_actions_adddup2(&myFA, myCoutPipe[1], 1);
  ret = posix_spawn_file_actions_adddup2(&myFA, myCerrPipe[1], 2);

  // Other way >> to file...
  // posix_spawn_file_actions_addopen(&a, 1, "/tmp/log", O_WRONLY | O_CREAT | O_TRUNC, 0644);
  // posix_spawn_file_actions_adddup2(&a, 1, 2);

  // I need args dynamic for general ability obviously
  std::vector<std::string> args; // FIXME: Does posix_spawnp need this memory preserved?
  std::vector<char *> cargs;
  for (size_t i = 0; i < argsin.size(); i++) {
    args.push_back(argsin[i]);
    cargs.push_back(&args[i][0]);
  }
  cargs.push_back(nullptr);

  // Spawn child with our ENV, close pipes
  ret = posix_spawnp(&myPid, &args[0][0], &myFA, NULL, &cargs[0], environ);
  close(myCoutPipe[1]);
  close(myCerrPipe[1]);

  // I think checking ret once at end is enough for our purposes,
  // since the final spawn should fail if anything in pipeline did.
  if (ret) {
    LogSevere("Spawned EXE watcher failed to call external process\n");
    return false;
  }

  LogInfo("Spawned EXE watcher \n");
  myConnected = true;
  return true;
} // EXEWatcher::EXEInfo::connect

bool
EXEWatcher::attach(const std::string & param,
  IOListener *                       l)
{
  // Guess we do the connection right?
  //
  std::shared_ptr<EXEInfo> newWatch = std::make_shared<EXEInfo>(l, param);
  bool success = newWatch->connect();

  if (success) {
    myWatches.push_back(newWatch);
  } else {
    LogSevere("Unable to connect to EXE watcher\n");
  }

  return success;
}

void
EXEWatcher::introduceSelf()
{
  std::shared_ptr<EXEWatcher> io = std::make_shared<EXEWatcher>();
  IOWatcher::introduce("exe", io);
}
