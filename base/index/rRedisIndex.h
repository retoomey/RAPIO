#pragma once

#include <rIndexType.h>
#include <rIOIndex.h>
#include <rURL.h>

namespace rapio {
/**
 * A index that watches a Redis channel for FML records
 * and maybe commands in the future.
 *
 * Could be a centralized option to avoid ldm
 * interconnected graphing.
 *
 * hiredis is on most distros now
 * FIXME: Not sure handles server up/down reconnect yet.
 * FIXME: Configure/parse server ip, port, etc.
 *
 * @author Robert Toomey
 * @ingroup rapio_io
 * @brief Index that uses a Redis channel for FML records
 */
class RedisIndex : public IndexType {
public:

  /** Default constant for a Redis index */
  static constexpr const char * REDISINDEX = "iredis";

  /** Create a Redis index */
  RedisIndex(){ }

  /** Destroy a Redis index and clean up */
  virtual
  ~RedisIndex();

  /** Get help for us */
  virtual std::string
  getHelpString(const std::string& fkey) override;

  /** Can we handle this protocol/path from -i?  Update allowed. */
  static bool
  canHandle(const URL& url, std::string& protocol, std::string& indexparams);

  // Factory methods ------------------------------------------

  /** Create an individual Redis index connection */
  virtual std::shared_ptr<IndexType>
  createIndexType(
    const std::string  & protocol,
    const std::string  & location,
    const TimeDuration & maximumHistory) override;

  /** Introduce to list of indexes available */
  static void
  introduceSelf();

  /** Params are passed to the builder to use to
   * determine what test/datatype to build.
   */
  RedisIndex(const std::string& params,
    const TimeDuration        & maximumHistory);

  // Index methods ------------------------------------------
  /** Handle realtime vs. archive mode stuff */
  virtual bool
  initialRead(bool realtime, bool archive) override;

  /** Handle a new message from a watcher.  We're allowed to do work here. */
  virtual void
  handleNewEvent(WatchEvent * w) override;

private:

  /** The params passed to us for sending to builder */
  std::string myParams;
};
}
