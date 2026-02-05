#include "rError.h"

#include "rStrings.h" // for toLower()
#include "rEventLoop.h"
#include "rEventTimer.h"
#include "rIOURL.h"
#include "rSignals.h"

#include <algorithm>
#include <iostream>

#include <rColorTerm.h>
#include <rTime.h>

#include <rFactory.h>
using namespace rapio;
using namespace std;

std::shared_ptr<Logger> Log::myLog;

std::mutex logMutex;

bool Log::useColors     = false;
bool Log::useHelpColors = false;

std::vector<int> Log::outputtokens;
std::vector<LogToken> Log::outputtokensENUM;
std::vector<std::string> Log::fillers;

// Quick flags we update when mode/pausing changes
Log::Severity Log::myCurrentLevel = Log::Severity::INFO;
bool Log::wouldDebug  = false; // start false until checked
bool Log::wouldInfo   = false; // start false until checked
bool Log::wouldSevere = false; // start false until checked

std::shared_ptr<LogSettingURLWatcher> Log::myLogURLWatcher = nullptr;

int Log::myPausedLevel = 0;
int Log::engine        = 0; // cout
int Log::myFlushMS     = 900;

namespace {
/** Utility to convert from a string to a level */
bool
stringToSeverity(const std::string& level, Log::Severity& severity)
{
  std::string s = level;

  Strings::toLower(s);
  if (s == "") { s = "info"; }

  if (s == "severe") {
    severity = Log::Severity::SEVERE;
  } else if (s == "info") {
    severity = Log::Severity::INFO;
  } else if (s == "debug") {
    severity = Log::Severity::DEBUG;
  } else {
    return false;
  }
  return true;
}

/** Utility to convert from a Log::Severity to a string */
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

void
Log::initialize()
{
  if (myLog == nullptr) {
    // Look for logging modules...
    Factory<Logger>::introduceLazy("spd", "librapiologspd.so", "createRAPIOLOG");
    myLog = Factory<Logger>::get("spd", "Logger: spd loading");

    // Set fallback log pattern
    setLogPattern("[%TIME%] %MESSAGE%");

    // Set fallback flush interval
    myFlushMS = 3000;
    myLog->setFlushMilliseconds(myFlushMS);
  }
}

void
Log::syncFlags()
{
  wouldInfo   = (myLog != nullptr) && (Severity::INFO >= myCurrentLevel) && (myPausedLevel <= 0);
  wouldDebug  = (myLog != nullptr) && (Severity::DEBUG >= myCurrentLevel) && (myPausedLevel <= 0);
  wouldSevere = (myLog != nullptr) && (Severity::SEVERE >= myCurrentLevel) && (myPausedLevel <= 0);
  if (myLog) {
    // FIXME: Maybe we increase our logging level ability
    switch (myCurrentLevel) {
        case Severity::DEBUG:
          myLog->setLoggingLevel(LogLevel::Debug);
          break;
        case Severity::INFO:
          myLog->setLoggingLevel(LogLevel::Info);
          break;
        case Severity::SEVERE:
          myLog->setLoggingLevel(LogLevel::Error);
          break;
    }
  }
}

void
Log::pauseLogging()
{
  // lock to sync?
  ++myPausedLevel;
  syncFlags();
}

void
Log::restartLogging()
{
  // lock to sync?
  --myPausedLevel;
  syncFlags();
}

bool
Log::isPaused()
{
  return (myPausedLevel > 0);
}

void
Log::flush()
{
  if (myLog) {
    myLog->flush();
  }
}

void
Log::setLogPattern(const std::string& pattern)
{
  std::vector<std::string> tokens =
  { "%TIME%",   "%TIMEMS%", "%TIMEC%", "%MESSAGE%", "%FILE%",   "%LINE%", "%FUNCTION%",
    "%LEVEL%",
    "%ECOLOR%", "%RED%",    "%GREEN%", "%BLUE%",    "%YELLOW%", "%CYAN%",
    "%OFF%",    "%THREADID%" };
  // Output
  std::vector<int> outputtokens;    // Token numbers in order
  std::vector<std::string> fillers; // Stuff between tokens

  Strings::TokenScan(pattern, tokens, outputtokens, fillers);

  Log::fillers = fillers;

  // Convert numbers to enum class
  outputtokensENUM.clear();
  for (auto& a:outputtokens) {
    // Assuming enums always order from 0
    if (a == -1) {
      outputtokensENUM.push_back(LogToken::filler);
    } else {
      outputtokensENUM.push_back(static_cast<LogToken>(a + 1));
    }
  }
  if (myLog != nullptr) {
    myLog->setTokenPattern(Log::outputtokensENUM);
  }
}

void
LogSettingURLWatcher::action()
{
  bool found = false;

  Log::Severity severity = Log::Severity::SEVERE; // fall back

  std::vector<char> buf;

  if (IOURL::read(myURL, buf) > 0) {
    // For first attempt, we just find the first line of text that matches
    // a logging level and use it.  Granted we could modify lots of stuff
    buf.push_back('\0');
    std::istringstream is(&buf.front());
    std::string s;
    while (getline(is, s, '\n')) {
      Strings::trim(s);
      if (stringToSeverity(s, severity)) {
        found = true;
        break;
      }
    }
  }
  if (!found) {
    fLogSevere("Can't set log level from  \"{}\", using severe.", myURL.toString());
  }
  // Set the level, fall back to severe if the level url is wrong
  Log::setSeverity(severity);
}

void
Log::setSeverity(Log::Severity severity)
{
  // Update severity level to given
  static bool firstTime = true;

  if (firstTime || (severity != myCurrentLevel)) {
    myCurrentLevel = severity; // lock to sync all the flags?
    syncFlags();

    const bool enableStackTrace = (severity == Severity::DEBUG);
    const bool wantCoreDumps    = (severity == Severity::DEBUG);
    Signals::initialize(enableStackTrace, wantCoreDumps);
    firstTime = false;
  }
}

void
Log::setSeverityString(const std::string& level)
{
  // Does the string match a known level...
  Severity severity = Severity::SEVERE;

  if (!stringToSeverity(level, severity)) {
    // .. if not try to use it as a URL...
    if (myLogURLWatcher == nullptr) {
      // Set up a timer to auto try the possible URL every so often.
      // FIXME: No way to configure timer at moment.  We need to balance
      // between hammering OS and taking too long to update.  For now change every 30 seconds
      std::shared_ptr<LogSettingURLWatcher> watcher = std::make_shared<LogSettingURLWatcher>(level, 30000);
      EventLoop::addEventHandler(watcher);
      myLogURLWatcher = watcher;
    } else {
      myLogURLWatcher->setURL(level);
    }
    myLogURLWatcher->action(); // Force watcher to update from file now
  } else {
    setSeverity(severity);
  }
}

void
Log::setLogEngine(const std::string& newengine)
{
  // Do nothing for now.  Only spd logger
}

void
Log::setLogFlushMilliSeconds(int newflush)
{
  myFlushMS = newflush;
  myLog->setFlushMilliseconds(newflush);
}

void
Log::setLogUseColors(bool useColor)
{
  useColors = useColor;
}

void
Log::setHelpColors(bool useColor)
{
  useHelpColors = useColor;
}

void
Log::printCurrentLogSettings()
{
  // Show current settings we're using
  std::string realLevel;

  if (myLogURLWatcher != nullptr) {
    realLevel = "From file:" + myLogURLWatcher->getURL().toString();
  } else {
    realLevel = severityToString(myCurrentLevel);
  }
  fLogInfo("Current Logging Settings Level: {} Flush: {} milliseconds.", realLevel, myFlushMS);
}
