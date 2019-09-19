#include "rError.h"

#include "rStrings.h" // for toLower()
#include "rEventLoop.h"
#include "rEventTimer.h"
#include "rBuffer.h"
#include "rIOURL.h"
#include "rSignals.h"

#include <algorithm>
#include <iostream>

#include <rColorTerm.h>

#include <boost/log/expressions.hpp>
#include <boost/format.hpp>

using namespace rapio;
using namespace std;

namespace logging = boost::log;
namespace src     = boost::log::sources;
namespace expr    = boost::log::expressions;
namespace sinks   = boost::log::sinks;

long Log::bytecounter = 0;
boost::log::sources::severity_logger<boost::log::trivial::severity_level> Log::mySevLog;

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

bool
Log::isPaused()
{
  return (instance()->myPausedLevel > 0);
}

void
Log::flush()
{
  instance()->mySink->locked_backend()->flush();
}

void
my_formatter(logging::record_view const& rec, logging::formatting_ostream& strm)
{
  // Allow pausing of logging in levels
  if (Log::instance()->isPaused()) { return; }

  // Get the LineID attribute value and put it into the stream
  // strm << logging::extract< unsigned int >("LineID", rec)  << ":";


  // Severity level.
  // strm << "\033[32m" << boost::format("%6s") % rec[logging::trivial::severity] << ":";
  strm << rec[logging::trivial::severity] << ":";
  // strm << "\033[32m" << "<" << rec[logging::trivial::severity] << "> ";

  // __FILE__ attribute
  logging::value_ref<std::string> fullpath = logging::extract<std::string>("File", rec);
  // strm << boost::filesystem::path(fullpath.get()).filename().string() << ":";
  strm << boost::format("%10s") % boost::filesystem::path(fullpath.get()).filename().string() << ":";

  // strm << "\033[0m";

  // __LINE__ attribute
  strm << logging::extract<int>("Line", rec) << ": ";

  // Finally, put the record message to the stream
  strm << rec[expr::smessage];
}

/** Hack around boost missing class? */
namespace boost
{
BOOST_LOG_OPEN_NAMESPACE
struct empty_deleter {
  typedef void result_type;
  void
  operator () (const volatile void *) const { }
};

BOOST_LOG_CLOSE_NAMESPACE
}

Log::Log()
  :
  myCurrentLevel(Severity::SEVERE),
  myPausedLevel(0)
{
  // init sink
  mySink = boost::make_shared<text_sink>();

  // We could do files and autorotation all with boost if wanted...
  // ..but we use our runner scripting
  // sink->locked_backend()->add_stream(
  //    boost::make_shared< std::ofstream >("sample.log"));

  // We have to provide an empty deleter to avoid destroying the global stream object
  boost::shared_ptr<std::ostream> stream(&std::clog, boost::log::v2_mt_posix::empty_deleter());
  mySink->locked_backend()->add_stream(stream);

  // We flush on a timer
  mySink->locked_backend()->auto_flush(false);
  mySink->set_formatter(&my_formatter);
  logging::core::get()->add_sink(mySink);

  logging::add_common_attributes();
}

Log::~Log()
{ }

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
        int flush = myLogFlusher->getTimerMilliseconds();
        setLogSettings(severity, flush);
        break;
      }
    }
  }
  return found;
}

void
Log::setLogSettings(Log::Severity severity, int flush)
{
  // Update severity level to given
  if (severity != myCurrentLevel) {
    myCurrentLevel = severity;

    // BOOST ONLY
    switch (myCurrentLevel) {
        case Log::Severity::SEVERE:
          logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::error);
          break;

        case Log::Severity::INFO:
          logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::info);
          break;

        case Log::Severity::DEBUG:
          logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::debug);
          break;
          // We want compile fail so no default
    }

    const bool enableStackTrace = (severity == Severity::DEBUG);
    const bool wantCoreDumps    = (severity == Severity::DEBUG);
    Signals::initialize(enableStackTrace, wantCoreDumps);
  }

  // Update flush timer
  myLogFlusher->setTimerMilliseconds(flush);
}

void
Log::setInitialLogSettings(const std::string& level, int flush)
{
  // Set up a timer to auto flush logs
  std::shared_ptr<LogFlusher> flusher = make_shared<LogFlusher>(flush);
  EventLoop::addTimer(flusher);
  myLogFlusher = flusher;

  // Set up a timer to auto try the possible URL every so often.
  // NOTE: initial action on watcher here requires above work done first
  Severity severity = Severity::SEVERE;
  if (stringToSeverity(level, severity)) { // parsed ok
    // LogSevere("Log setting parsed ok " << severityToString(severity) << "\n");
  } else {
    // Set up a timer to auto try the possible URL every so often.
    // FIXME: No way to configure timer at moment.  We need to balance
    // between hammering OS and taking too long to update.  For now change every 30 seconds
    std::shared_ptr<LogSettingURLWatcher> watcher = std::make_shared<LogSettingURLWatcher>(level, 30000);
    EventLoop::addTimer(watcher);
    myLogURLWatcher = watcher;
    watcher->action(); // Do initial set.
  }
  // FORCED first time
  myCurrentLevel = severity;
  // BOOST ONLY
  switch (myCurrentLevel) {
      case Log::Severity::SEVERE:
        logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::error);
        break;

      case Log::Severity::INFO:
        logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::info);
        break;

      case Log::Severity::DEBUG:
        logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::debug);
        break;
        // We want compile fail so no default
  }
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
                    << "Flush: " << myLogFlusher->getTimerMilliseconds() << " milliseconds. \n");
} // Log::setInitialLogSettings
