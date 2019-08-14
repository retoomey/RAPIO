#include "rError.h"

#include "rStrings.h" // for toLower()
#include "rEventLoop.h"
#include "rEventTimer.h"

#include <algorithm>
#include <iostream>

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

Streams&
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

  if (s == "severe") {
    severity = Severe;
  } else if (s == "info") {
    severity = Info;
  } else if (s == "debug") {
    severity = Debug;
  } else {
    severity = Severe;
    LogSevere("Unknown verbosity level \"" << s << "\".  Use \"severe\", \"info\", \"debug\"\n");
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

Streams::Streams()
{ }

size_t
Streams::addStream(std::ostream * os)
{
  for (auto& i:myStreams) {
    if (i == os) {
      return 1;
    }
  }
  myStreams.push_back(os);
  return 1;
}

void
Streams::removeStream(std::ostream * os)
{
  myStreams.erase(std::remove(myStreams.begin(), myStreams.end(), os), myStreams.end());
}

void
Streams::cycle(size_t myCycleSize)
{
  for (auto& s:myStreams) {
    if (s->tellp() > (int) myCycleSize) {
      // gotta cycle now
      // time_t now = time(0);
      // (*(s)) << "\n\n\nLOG cycled. Rest of info is OLD.\n";
      s->flush();
      s->seekp(ios::beg);
      // (*(s)) << "LOG cycled at "
      //       << ctime(&now)
      //       << "\n";
    }
  }
}

void
Streams::flush()
{
  for (auto& s:myStreams) {
    s->flush();
  }
}
