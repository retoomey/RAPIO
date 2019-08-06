#include "rError.h"

#include "rStrings.h" // for toLower()
#include "rEventLoop.h"
#include "rRAPIOAlgorithm.h"
#include "rEventTimer.h"

#include <cassert>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

using namespace rapio;
using namespace std;

void
Log::startLogging(std::ostream * s)
{
  instance()->myLogStream.addStream(s);
}

void
Log::stopLogging(std::ostream * s)
{
  instance()->myLogStream.removeStream(s);
}

void
Log::pauseLogging()
{
  ++(instance()->myPausedLevel);
}

void
Log::restartLogging()
{
  --(instance()->myPausedLevel);
}

void
Log::flush()
{
  instance()->myLogStream.flush();
}

Log::Log()
  :
  myCurrentLevel(Severe),
  myPausedLevel(0),
  myCycleSize(-1)
{
  // Set the time for future use.
  myLogStream.addStream(&std::cout);

  // the null stream is empty so the loop in << will not be executed
}

Log::~Log()
{ }

Log::Streams&
Log::get(Log::Severity req_level)
{
  Log * log = instance();

  // should we cycle?
  if (log->myCycleSize > 0) {
    log->myLogStream.cycle(log->myCycleSize);
  }

  // is it severe enough?
  if (req_level < log->myCurrentLevel) {
    return (log->myNullStream);
  }

  // are we myPausedLevel?
  if (log->myPausedLevel > 0) {
    return (log->myNullStream);
  }

  return (log->myLogStream);
}

void
Log::setLogSize(int newSize)
{
  instance()->myCycleSize = newSize;
}

namespace rapio {
class LogFlusher : public rapio::EventTimer {
public:

  LogFlusher(size_t milliseconds) : EventTimer(milliseconds, "Log flusher")
  { }

  virtual void
  action() override
  {
    // LogSevere("Flushing log...\n");
    Log::flush();
  }
};
}

void
Log::setVerbose(const std::string& in)
{
  Severity severity(Severe);

  std::string s = in;
  Strings::toLower(s);

  // FIXME: enum class?
  if (s == "severe") {
    severity = Severe;
  } else if (s == "unanticipated") {
    severity = NotAnticipated;
  } else if (s == "harmless") {
    severity = NotSevere;
  } else if (s == "important") {
    severity = ImpInfo;
  } else if (s == "info") {
    severity = Info;
  } else if (s == "debug") {
    severity = Debug;
  } else {
    severity = Severe;
    LogSevere("Unknown verbosity level \"" << s << "\".  Use \"severe\", "
                                           << "\"unanticipated\", \"harmless\", \"important\", \"info\", \"debug\", "
                                           << "\n");
  }
  setSeverityThreshold(severity);
}

void
Log::setLogSettings(const std::string& level, int flush, int logSize)
{
  /** Set level first so future log messages show */
  setVerbose(level);
  instance()->setLogSize(logSize);

  /** Set up a timer to auto flush logs */
  std::shared_ptr<LogFlusher> flusher(new LogFlusher(flush));
  EventLoop::addTimer(flusher);

  LogInfo("Level: " << level << " "
                    << "Flush: " << flush << " seconds. "
                    << "Cycle: " << instance()->myCycleSize << " bytes.\n");
}

Log::Streams::Streams()
{ }

size_t
Log::Streams::addStream(std::ostream * os)
{
  // the [] operator inserts a zero if os isn't there already.
  int number = (++myStreams[os]);

  assert(number > 0);
  return (number);
}

size_t
Log::Streams::removeStream(std::ostream * os)
{
  int number = 0;

  StreamSet::iterator iter = myStreams.find(os);

  if (iter ==
    myStreams.end())
  {
    std::cerr
      << "WARNING! Trying to remove non-existent stream!\n";
  } else {
    number = --(iter->second);
    assert(number >= 0);

    if (number == 0) { myStreams.erase(iter); }
  }
  return (number);
}

void
Log::Streams::cycle(size_t myCycleSize)
{
  for (auto& i:myStreams) {
    std::ostream * this_stream = i.first;

    if (this_stream->tellp() > (int) myCycleSize) {
      // gotta cycle now
      time_t now = time(0);
      (*(this_stream)) << "\n\n\nLOG cycled. Rest of info is OLD.\n";
      this_stream->flush();
      this_stream->seekp(ios::beg);
      (*(this_stream)) << "LOG cycled at "
                       << ctime(&now)
                       << "\n";
    }
  }
}

void
Log::Streams::flush()
{
  for (auto& i:myStreams) {
    std::ostream * this_stream = i.first;
    this_stream->flush();
  }
}
