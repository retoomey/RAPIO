#include "rRedisIndex.h"

#include "rError.h"
#include "rRecordQueue.h"
#include "rRedisWatcher.h"
#include "rIODataType.h"
#include "rPTreeData.h"
#include "rConfigRecord.h"

using namespace rapio;

void
RedisIndex::introduceSelf()
{
  std::shared_ptr<IndexType> newOne = std::make_shared<RedisIndex>();

  IOIndex::introduce(REDISINDEX, newOne);
}

RedisIndex::RedisIndex(const std::string & params,
  const TimeDuration                     & maximumHistory) :
  IndexType(maximumHistory),
  myParams(params)
{ }

RedisIndex::~RedisIndex()
{ }

std::string
RedisIndex::getHelpString(const std::string& fkey)
{
  #ifdef HAVE_REDIS
  return "Use a Redis channel for .fml metadata files.";

  #else
  return "(unavailable) Use a Redis channel for .fml metadata files.";

  #endif
}

bool
RedisIndex::canHandle(const URL& url, std::string& protocol, std::string& indexparams)
{
  return false;
} // RedisIndex::canHandle

bool
RedisIndex::initialRead(bool realtime, bool archive)
{
  if (realtime) {
    // There's no 'history' with redis.  Real-time only support
    std::shared_ptr<WatcherType> watcher = IOWatcher::getIOWatcher(RedisWatcher::REDIS_WATCH);

    bool ok = watcher->attach(myParams, realtime, archive, this);
    if (!ok) { return false; }
  } else {
    fLogSevere("Redis index is realtime only.  Use -r if you want to use it.");
  }
  return true;
}

void
RedisIndex::handleNewEvent(WatchEvent * event)
{
  const std::string& payload = event->myData;
  bool isFML = false;

  // Try to parse as FML/XML Record
  try {
    // IODataType reads from vector<char>
    std::vector<char> buffer(payload.begin(), payload.end());

    auto xml = IODataType::readBuffer<PTreeData>(buffer, "xml");

    if (xml) {
      // FML records are <item>...</item>
      auto tree = xml->getTree();

      // Use getChild which throws if not found, catching it below is fine
      // or use getChildOptional if you prefer safer checks.
      // The logic in rStreamIndex uses getChild.
      auto item = tree->getChild("item");

      Record rec;
      // We pass "" as indexPath because Redis records likely won't rely on
      // relative file paths from the index location.
      if (ConfigRecord::readXML(rec, item, "", getIndexLabel())) {
        Record::theRecordQueue->addRecord(rec);
        //   fLogInfo("Redis FML Record Queued: {}", rec.getIDString());
        isFML = true;
      }
    }
  } catch (...) {
    // If it fails to parse as XML or find the <item> tag, it's not a standard FML record.
    // Fall through to print raw message.
    isFML = false;
  }

  // Handle Non-FML
  if (!isFML) {
    // Just print the raw payload
    fLogInfo("Redis Message (Raw): {}", payload);
  }
} // RedisIndex::handleNewEvent

std::shared_ptr<IndexType>
RedisIndex::createIndexType(
  const std::string  & protocol,
  const std::string  & indexparams,
  const TimeDuration & maximumHistory)
{
  std::shared_ptr<RedisIndex> result = std::make_shared<RedisIndex>(indexparams,
      maximumHistory);

  return (result);
}
