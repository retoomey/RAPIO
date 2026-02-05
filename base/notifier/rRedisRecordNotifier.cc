#include "rRedisRecordNotifier.h"
#include "rError.h"
#include "rFactory.h"
#include "rConfigRecord.h" // For XML string construction

#ifdef HAVE_HIREDIS
# include <hiredis/hiredis.h>
#endif
#include <sstream>

using namespace rapio;

namespace rapio {
/** Creator class registered with factory.
 * decouples any memory of the Notifier from creation */
class RedisRecordNotifierCreator : public RecordNotifierCreator {
public:
  std::shared_ptr<RecordNotifierType>
  create(const std::string& params) override
  {
    auto ptr = std::make_shared<RedisRecordNotifier>();

    ptr->initialize(params);
    return ptr;
  }

  virtual std::string
  getHelpString(const std::string& fkey) override
  {
    #ifdef HAVE_REDIS
    return "Publish FML records to a Redis channel.\n  Example: redis=my_channel";

    #else
    return "(unavailable) Publish FML records to a Redis channel.\n  Example: redis=my_channel";

    #endif
  }
};
}

void
RedisRecordNotifier::introduceSelf()
{
  std::shared_ptr<RedisRecordNotifierCreator> newOne = std::make_shared<RedisRecordNotifierCreator>();
  Factory<RecordNotifierCreator>::introduce("redis", newOne);
}

RedisRecordNotifier::RedisRecordNotifier() :
  myChannel("rapio"), // FIXME: make configurable
  myHostname("127.0.0.1"),
  myPort(6379),
  myContext(nullptr),
  myConnected(false)
{ }

RedisRecordNotifier::~RedisRecordNotifier()
{
  #ifdef HAVE_REDIS
  if (myContext) {
    redisFree((redisContext *) myContext);
    myContext = nullptr;
  }
  #endif
}

void
RedisRecordNotifier::initialize(const std::string& params)
{
  // Params passed from -n=redis=PARAMS
  if (!params.empty()) {
    myChannel = params;
  }

  fLogInfo("Redis Notifier initialized for channel: {}", myChannel);
  connect();
}

bool
RedisRecordNotifier::connect()
{
  #ifdef HAVE_REDIS
  if (myConnected) { return true; }

  redisContext * c = redisConnect(myHostname.c_str(), myPort);

  if ((c == nullptr) || c->err) {
    if (c) {
      fLogSevere("Redis Connection Error: {}", c->errstr);
      redisFree(c);
    } else {
      fLogSevere("Redis Connection Error: Can't allocate context");
    }
    myContext   = nullptr;
    myConnected = false;
    return false;
  }

  fLogInfo("Redis Notifier connected to {}:{}", myHostname, myPort);
  myContext   = (void *) c;
  myConnected = true;
  return true;

  #else // ifdef HAVE_REDIS
  return false;

  #endif // ifdef HAVE_REDIS
}

void
RedisRecordNotifier::writeMessage(std::map<std::string, std::string>& outputParams, const Message& m)
{
  // Wrap message in a record and send
  Record r(m);

  writeRecord(outputParams, r);
}

void
RedisRecordNotifier::publish(const std::string& payload)
{
  if (!connect()) { return; }

  #ifdef HAVE_REDIS
  redisContext * c = (redisContext *) myContext;

  // PUBLISH channel message
  redisReply * reply = (redisReply *) redisCommand(c, "PUBLISH %s %s", myChannel.c_str(), payload.c_str());

  if (reply == nullptr) {
    fLogSevere("Redis PUBLISH failed, lost connection?");
    redisFree(c);
    myContext   = nullptr;
    myConnected = false;
    return;
  }

  // PUBLISH returns an integer (number of clients received), usually we don't care
  // but we must free the reply object to prevent leaks.
  freeReplyObject(reply);
  #endif // ifdef HAVE_REDIS
}

void
RedisRecordNotifier::writeRecord(std::map<std::string, std::string>& outputParams, const Record& rec)
{
  // Construct the XML payload
  std::stringstream ss;

  ss << "<item>\n";

  // We pass "redis" as the index path, or you could pass outputParams["outputfolder"]
  // if you want the path in the XML to reflect the data location
  std::string locationHint = "redis";

  if (outputParams.count("outputfolder")) {
    locationHint = outputParams["outputfolder"];
  }

  ConfigRecord::constructXMLString(rec, ss, locationHint);
  ss << "</item>\n";

  // Publish to Redis
  publish(ss.str());
}
