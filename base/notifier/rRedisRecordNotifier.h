#pragma once
#include <rRecordNotifier.h>
#include <string>

namespace rapio {
/**
 * Notify FML records to a Redis server.
 *
 * @author Robert Toomey
 * @ingroup rapio_io
 * @brief Notify FML records to a Redis server
 */
class RedisRecordNotifier : public RecordNotifierType {
public:

  /** Register with the RecordNotifier classes */
  static void
  introduceSelf();

  /** Construct a new RedisRecordNotifier */
  RedisRecordNotifier();

  /** Destruct a RedisRecordNotifier, making sure context is deleted */
  virtual
  ~RedisRecordNotifier();

  /** Write a message object to our queue for main loop processing */
  virtual void
  writeMessage(std::map<std::string, std::string>& outputParams, const Message& m) override;

  /** Write a record object to our queue for main loop processing */
  virtual void
  writeRecord(std::map<std::string, std::string>& outputParams, const Record& rec) override;

  /** Do any initialization this plugin requires */
  virtual void
  initialize(const std::string& params) override;

protected:

  /** Connect to a Redis server */
  bool
  connect();

  /** Send a message to a Redis server on our channel(s) */
  void
  publish(const std::string& payload);

  /** The Redis channel we're using */
  std::string myChannel; // FIXME: Maybe a vector, multi-channel?

  /** The Redis server ip/name */
  std::string myHostname;

  /** The Redis port number */
  int myPort;

  /** Void pointer to hide hiredis dependency from header, redis context */
  void * myContext;

  /** Are we currently connected? */
  bool myConnected;
};
}
