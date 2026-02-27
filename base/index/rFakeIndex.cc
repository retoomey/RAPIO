#include "rFakeIndex.h"

#include "rError.h"
#include "rRecordQueue.h"
#include "rFactory.h"
#include "rIOFakeDataGenerator.h"

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
    fLogInfo("Fake index archiving mode.  Generating.");
    // FIXME: Could ask generator for suggested time gap?
    // and maybe start time, etc.
    Time startTime(2026, 2, 24, 21, 12, 31, 0.585);

    size_t count = 140;
    for (size_t i = 0; i < count; ++i) {
      generateRecord(startTime);
      startTime += TimeDuration::Seconds(21); // Average vcp 212 time
    }
  }
  return true;
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
  // For the moment use the radar sim
  auto gen = Factory<IOFakeDataGenerator>::get("radar1");

  if (gen) {
    auto rec = gen->generateRecord(myParams, time);
    Record::theRecordQueue->addRecord(rec);
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
