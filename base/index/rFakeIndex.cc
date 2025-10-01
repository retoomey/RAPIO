#include "rFakeIndex.h"

#include "rError.h"
#include "rRecordQueue.h"
// Watchers usually watch and send heartbeat messages.
// We have two choices.  Make a FakeWatcher to do all the
// heartbeating, or just make a timer ourselves. Using
// a watcher might be nice we could trigger testing say with
// a web interface or something later.
// #include "rFakeIndexWatcher.h"

using namespace rapio;

/** Default constant for a fake index */
const std::string FakeIndex::FAKEINDEX = "fake";

void
FakeIndex::introduceSelf()
{
  std::shared_ptr<IndexType> newOne = std::make_shared<FakeIndex>();

  IOIndex::introduce(FAKEINDEX, newOne);
}

FakeIndex::FakeIndex(const std::string & params,
  const TimeDuration                   & maximumHistory) :
  IndexType(maximumHistory),
  myParams(params)
{ }

FakeIndex::~FakeIndex()
{ }

std::string
FakeIndex::getHelpString(const std::string& fkey)
{
  return "Fake data generator.  Can create artifcial data sets for testing purposes.";
}

bool
FakeIndex::canHandle(const URL& url, std::string& protocol, std::string& indexparams)
{
  // I think we can't read .fake or something like that.  Files don't exist
  //  if (protocol.empty() || (protocol == FAKEINDEX)) {
  //    return true;
  //  }
  return false;
} // FakeIndex::canHandle

bool
FakeIndex::initialRead(bool realtime, bool archive)
{
  if (archive) {
    LogInfo("Fake index archiving mode.  Generating.\n");
    // FIXME: Any 'fake data module class' will have to be callable by
    // the index here as well as the fake builder.
    // Start with real clock time (assuming no other archive indexes)
    Time time = Time::ClockTime();
    size_t volumeCount = 14;
    size_t numVols     = 10;
    for (size_t i = 0; i < volumeCount * numVols; ++i) {
      generateRecord(time);
      time += TimeDuration::Seconds(21); // Average vcp 212 time
    }
  }
  return true; // successful 'connection'
}

bool
FakeIndex::handlePoll()
{
  // Real time poll.  We force clock time here
  Time time = Time::ClockTime();

  return (generateRecord(time));
}

bool
FakeIndex::generateRecord(const Time& time)
{
  std::vector<std::string> params;
  std::string aBuilder = "fake";

  params.push_back(aBuilder);
  params.push_back(myParams);

  // Reflectivity VCP 212 to start
  // FIXME: We probably want a class to all of this eventually
  // to allow plugins
  std::vector<std::string> angleDegs = {
    "0.5", "0.9", "1.3", "1.8", "2.4", "3.1", "4.0", "5.1", "6.4", "8.0", "10.0", "12.5", "15.6", "19.5" };
  static size_t tilt = 0;
  Record rec(params, aBuilder, time, "Reflectivity", angleDegs[tilt]);

  Record::theRecordQueue->addRecord(rec);

  // if (tilt++ > angleDegs.size()){
  if (++tilt >= angleDegs.size()) {
    tilt = 0;
  }
  return true;
} // FakeIndex::readRemoteRecords

std::shared_ptr<IndexType>
FakeIndex::createIndexType(
  const std::string  & protocol,
  const std::string  & indexparams,
  const TimeDuration & maximumHistory)
{
  std::shared_ptr<FakeIndex> result = std::make_shared<FakeIndex>(indexparams,
      maximumHistory);

  return (result);
}
