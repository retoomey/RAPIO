#include "rError.h"

#include "rStrings.h" // for toLower()
#include "rEventLoop.h"
#include "rEventTimer.h"
#include "rBuffer.h"
#include "rIOURL.h"
#include "rSignals.h"

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
  myCurrentLevel(Severity::SEVERE),
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

void
LogFlusher::action()
{
  // LogSevere("Flushing log...\n");
  Log::flush();
}

void
LogSettingURLWatcher::action()
{
  if (Log::instance()->setFromURL(myURL)) { } else {
    LogSevere("Can't set log level from  \"" << myURL << "\"\n, using severe.\n");
  }
}

namespace {
bool
stringToSeverity(const std::string& level, Log::Severity& severity)
{
  std::string s = level;
  Strings::toLower(s);
  if (s == "") { s = "info"; }

  // Log::Severity severity = Log::Severity::SEVERE;
  // severity = Log::Severity::SEVERE;
  if (s == "severe") {
    severity = Log::Severity::SEVERE;
  } else if (s == "info") {
    severity = Log::Severity::INFO;
  } else if (s == "debug") {
    severity = Log::Severity::DEBUG;
  } else {
    // LogSevere("Unknown string '"<<s<<"', using log level severe\n");
    // severity = Log::Severity::SEVERE;
    return false;
  }
  return true;
}

std::string
severityToString(Log::Severity s)
{
  switch (s) {
      case Log::Severity::SEVERE: return "severe";

      case Log::Severity::INFO: return "info";

      case Log::Severity::DEBUG: return "debug";
        // We want compile fail so no default
  }
  return ""; // Make compiler happy
}
}

bool
Log::setFromURL(const URL& aURL)
{
  bool found = false;
  Buffer buf;

  if (IOURL::read(aURL, buf) > 0) {
    // LogSevere("Found log setting information at \"" << aURL << "\"\n");

    // For first attempt, we just find the first line of text that matches
    // a logging level and use it.  Granted we could modify lots of stuff
    buf.data().push_back('\0');
    std::istringstream is(&buf.data().front());
    std::string s;
    Log::Severity severity = Log::Severity::SEVERE;
    while (getline(is, s, '\n')) {
      Strings::trim(s);
      // LogSevere("Candidate string " << s << "\n");
      if (stringToSeverity(s, severity)) {
        // LogSevere("Found log setting from URL: " << severityToString(severity) << "\n");
        found = true;
        // FIXME: maybe develop the setting file more powerfully...
        // FIXME: Actually think doing a HMET style file would be good here
        int logSize = myCycleSize;
        int flush   = myLogFlusher->getTimerMilliseconds();
        setLogSettings(severity, flush, logSize);
        break;
      }
    }
  }
  return found;
}

void
Log::setLogSettings(Log::Severity severity, int flush, int logSize)
{
  // Update severity level to given
  if (severity != myCurrentLevel) {
    myCurrentLevel = severity;
    const bool enableStackTrace = (severity == Severity::DEBUG);
    const bool wantCoreDumps    = (severity == Severity::DEBUG);
    Signals::initialize(enableStackTrace, wantCoreDumps);
  }

  // Update the log size before flush
  setLogSize(logSize);

  // Update flush timer
  myLogFlusher->setTimerMilliseconds(flush);
}

void
Log::setInitialLogSettings(const std::string& level, int flush, int logSize)
{
  // Set the log size
  setLogSize(logSize);

  // Set up a timer to auto flush logs
  std::shared_ptr<LogFlusher> flusher(new LogFlusher(flush));
  EventLoop::addTimer(flusher);
  myLogFlusher = flusher;

  // Set up a timer to auto try the possible URL every so often.
  // NOTE: initial action on watcher here requires above work done first
  Severity severity = Severity::SEVERE;
  if (stringToSeverity(level, severity)) { // parsed ok
    LogSevere("Log setting parsed ok " << severityToString(severity) << "\n");
  } else {
    // Set up a timer to auto try the possible URL every so often.
    // FIXME: No way to configure timer at moment.  We need to balance
    // between hammering OS and taking too long to update.  For now change every 30 seconds
    std::shared_ptr<LogSettingURLWatcher> watcher(new LogSettingURLWatcher(level, 30000));
    EventLoop::addTimer(watcher);
    myLogURLWatcher = watcher;
    watcher->action(); // Do initial set.
  }
  // FORCED first time
  myCurrentLevel = severity;
  const bool enableStackTrace = (severity == Severity::DEBUG);
  const bool wantCoreDumps    = (severity == Severity::DEBUG);
  Signals::initialize(enableStackTrace, wantCoreDumps);

  // Get what was actually set...
  std::string realLevel;
  if (myLogURLWatcher != nullptr) {
    realLevel = myLogURLWatcher->getURL().toString();
  } else {
    realLevel = severityToString(myCurrentLevel);
  }
  LogInfo("Level: " << realLevel << " "
                    << "Flush: " << myLogFlusher->getTimerMilliseconds() << " milliseconds. "
                    << "Cycle: " << myCycleSize << " bytes.\n");
} // Log::setInitialLogSettings

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
