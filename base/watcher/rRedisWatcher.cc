#include "rRedisWatcher.h"

#include "rError.h" // for LogInfo()
#include "rOS.h"
#include "rStrings.h"

#include <queue>

#include <poll.h>     // poll
#include <sys/wait.h> // waitpid
#include <unistd.h>   // env

#ifdef HAVE_HIREDIS
# include <hiredis/hiredis.h>
#endif

using namespace rapio;

/** Default constant for a exe watcher */
const std::string RedisWatcher::REDIS_WATCH = "redis";

void
RedisWatcher::RedisInfo::createEvents(WatcherType * w)
{
  #ifdef HAVE_HIREDIS
  if (!myConnected || !myContext) {
    fLogSevere("Not connected to Redis server");
    connect();
    return;
  }

  redisContext * c = (redisContext *) (myContext);

  struct pollfd pfd;
  pfd.fd = c->fd;
  // FIX: Monitor POLLOUT to ensure the buffered SUBSCRIBE command gets sent.
  // Monitor POLLIN to receive server replies.
  pfd.events = POLLIN | POLLOUT;

  // Use 0 timeout because the WatcherType loop already manages the polling interval
  if (poll(&pfd, 1, 0) > 0) {
    // 1. Handle Writes (Flushing the output buffer)
    // If the socket is writable and we have data pending (like our SUBSCRIBE command), write it.
    if (pfd.revents & POLLOUT) {
      int done = 0;
      if (redisBufferWrite(c, &done) == REDIS_ERR) {
        fLogSevere("Redis write error, disconnecting...");
        myConnected = false;
        return;
      }
    }

    // 2. Handle Reads (Incoming Messages)
    if (pfd.revents & POLLIN) {
      // Feed data from socket into hiredis buffer
      if (redisBufferRead(c) != REDIS_OK) {
        fLogSevere("Redis read error, disconnecting...");
        myConnected = false;
        return;
      }

      // 3. Extract all completed messages from the reader buffer
      void * reply = nullptr;
      while (redisGetReplyFromReader(c, &reply) == REDIS_OK && reply != nullptr) {
        redisReply * r = (redisReply *) reply;

        // Redis Pub/Sub messages are arrays: ["message", channel, payload]
        if ((r->type == REDIS_REPLY_ARRAY) && (r->elements == 3)) {
          // r->element[0] is type (e.g., "message")
          // r->element[1] is the channel name
          // r->element[2] is the actual payload string

          if (r->element[1]->str && r->element[2]->str) {
            std::string channel = r->element[1]->str;
            std::string payload = r->element[2]->str;

            // PRINT: As requested, log the channel and message
            fLogInfo("RedisWatcher: Channel '{}' -> Payload: {}", channel, payload);

            // Create the event for the Index to process
            WatchEvent e(myListener, "redis_msg", payload);
            w->addEvent(e);
          }
        }
        freeReplyObject(r);
      }
    }
  }
  #endif // ifdef HAVE_HIREDIS
} // RedisWatcher::EXEInfo::createEvents

void
RedisWatcher::getEvents()
{
  // Watches can create events if the event creation process
  // is independent of other event processes.
  for (auto& w:myWatches) {
    w->createEvents(this);
  }
} // RedisWatcher::getEvents

bool
RedisWatcher::RedisInfo::connect()
{
  #ifdef HAVE_HIREDIS
  redisContext * c = nullptr;

  // 1. Connect to Redis
  c = redisConnect("127.0.0.1", 6379);
  if (!c || c->err) {
    fLogSevere("Redis connection error: {}", (c ? c->errstr : "can't allocate context"));
    return false;
  }

  // 2. Set to NON-BLOCKING mode
  // Hiredis handles the fcntl(O_NONBLOCK) logic internally via this call
  redisSetTimeout(c, (struct timeval){ 0, 0 });
  // int flags = fcntl(c->fd, F_GETFL, 0);
  // fcntl(c->fd, F_SETFL, flags | O_NONBLOCK);

  // 3. Send the SUBSCRIBE command
  // We don't wait for a reply here because we are in an async setup
  redisAppendCommand(c, "SUBSCRIBE %s", myParams.c_str());

  fLogInfo("Redis Watcher connected and subscribed to '{}'", myParams);
  myConnected = true;
  myContext   = (void *) (c);
  return true;

  #else // ifdef HAVE_HIREDIS
  // If you see this, CMake probably didn't find the libraries.
  // On redhat-based typically you need hiredis-devel
  fLogSevere("Redis support not available/compiled in. Probably missing libraries.");
  return false;

  #endif // ifdef HAVE_HIREDIS
} // RedisWatcher::EXEInfo::connect

bool
RedisWatcher::attach(const std::string & param,
  bool                                 realtime,
  bool                                 archive,
  IOListener *                         l)
{
  // Set up new RedisWatcher and connect
  std::shared_ptr<RedisInfo> newWatch = std::make_shared<RedisInfo>(l, param);
  bool success = newWatch->connect();

  if (success) {
    myWatches.push_back(newWatch);
  } else {
    LogSevere("Unable to connect to Redis watcher\n");
  }

  return success;
}

void
RedisWatcher::introduceSelf()
{
  std::shared_ptr<RedisWatcher> io = std::make_shared<RedisWatcher>();

  IOWatcher::introduce(REDIS_WATCH, io);
}
