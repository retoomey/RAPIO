#pragma once

#include <rIOWatcher.h>
#include <rEventLoop.h>
#include <rEventTimer.h>
#include <rURL.h>

#include <string>

namespace rapio {
/**
 * Redis watcher watches a Redis server/channel for messages
 *
 * @author Robert Toomey
 * @ingroup rapio_io
 * @brief Watches a Redis server/channel for messages
 */
class RedisWatcher : public WatcherType {
public:
  /** Default constant for a exe watcher */
  static const std::string REDIS_WATCH;

  /** Construct a RedisWatcher */
  RedisWatcher() : WatcherType(100, 2, "REDIS Watcher"){ }

  /** Introduce this to the global factory */
  static void
  introduceSelf();

  /** Information for a particular Redis watch */
  class RedisInfo : public WatchInfo {
public:
    friend RedisWatcher;

    /** Create the Redis info for a single connection */
    RedisInfo(IOListener * l, const std::string& params) : WatchInfo(l), myParams(params)
    { }

    /** Spawn and connect to the exe */
    bool
    connect();

    /** Create the events to be processed later */
    virtual void
    createEvents(WatcherType * w) override;

protected:

    /** redisContext* */
    void * myContext;

    /** Are we connected to the Redis server? */
    bool myConnected;

    /** Parameters used for calling the executable */
    std::string myParams;

    /** Channel this watch is connected to */
    std::string myChannel;
  };

  /** Attach a given listener to us */
  virtual bool
  attach(const std::string& dirname, bool realtime, bool archive, IOListener *) override;

  /** Get some events.  Depending on watcher this can be a global process or
   * passed on the individual infos */
  virtual void
  getEvents() override;

  /** Destroy us */
  virtual ~RedisWatcher(){ }
};
}
