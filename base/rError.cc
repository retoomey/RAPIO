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

#include <boost/log/expressions.hpp>
#include <boost/format.hpp>

using namespace rapio;
using namespace std;

namespace logging = boost::log;
namespace src     = boost::log::sources;
namespace expr    = boost::log::expressions;
namespace sinks   = boost::log::sinks;

std::shared_ptr<Log> Log::mySingleton;

boost::log::sources::severity_logger<boost::log::trivial::severity_level> Log::mySevLog;

bool Log::useColors               = false;
bool Log::useHelpColors           = false;
std::string Log::LOG_TIMESTAMP    = "%Y %m/%d %H:%M:%S UTC";
std::string Log::LOG_TIMESTAMP_MS = "%Y %m/%d %H:%M:%S.%/ms UTC";

std::vector<int> Log::outputtokens;
std::vector<LogToken> Log::outputtokensENUM;
std::vector<std::string> Log::fillers;

Log::Severity Log::myCurrentLevel = Log::Severity::INFO;
Log::Severity Log::mode = Log::Severity::INFO;
int Log::line             = 0;
std::string Log::file     = "None";
std::string Log::function = "None";
std::stringstream Log::buffer;
int Log::myPausedLevel = 0;
int Log::engine        = 0; // cout

namespace {
/** Utility to convert from a string to a level */
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

namespace {
// Dump using a given stream
template <class T> void
dump(T& strm)
{
  // Logging is configurable by pattern string
  size_t fat = 0;

  for (auto a:Log::outputtokensENUM) {
    switch (a) {
        case LogToken::filler:
          strm << Log::fillers[fat++];
          break;
        case LogToken::time:
          strm << Time::CurrentTime().getString(Log::LOG_TIMESTAMP);
          break;
        case LogToken::timems:
          strm << Time::CurrentTime().getString(Log::LOG_TIMESTAMP_MS);
          break;
        case LogToken::message:
          // strm << rec[expr::smessage];
          strm << Log::buffer.str();
          break;
        case LogToken::file: {
          //   logging::value_ref<std::string> fullpath = logging::extract<std::string>("File", rec);
          //   strm << boost::format("%10s") % boost::filesystem::path(fullpath.get()).filename().string();
          //   strm << boost::filesystem::path(fullpath.get()).filename().string();
          strm << Log::file;
        }
        break;
        case LogToken::line:
          // strm << logging::extract<int>("Line", rec);
          strm << Log::line;
          break;
        case LogToken::function:
          // strm << logging::extract<std::string>("Function", rec);
          strm << Log::function;
          break;
        case LogToken::level:
          // strm << boost::format("%6s") % rec[logging::trivial::severity];
          // strm << rec[logging::trivial::severity];
          //      strm << rec[logging::trivial::severity];
          if (Log::mode == Log::Severity::INFO) { // Use enums I think...
            strm << "info";
          } else if (Log::mode == Log::Severity::SEVERE) {
            strm << "error";
          } else {
            strm << "debug";
          }
          break;
        case LogToken::ecolor: // Change depending on verbose level
          if (Log::useColors) {
            if (Log::mode == Log::Severity::INFO) {    // Use enums I think...
              strm << "\033[38;2;0;255;0;48;2;0;0;0m"; // green
            } else if (Log::mode == Log::Severity::SEVERE) {
              strm << "\033[38;2;255;0;0;48;2;0;0;0m"; // red
            } else {
              strm << "\033[38;2;0;255;255;48;2;0;0;0m"; // cyan
            }
          }
          break;
        case LogToken::red:
          if (Log::useColors) {
            strm << "\033[38;2;255;0;0;48;2;0;0;0m";
          }
          break;
        case LogToken::blue:
          if (Log::useColors) {
            strm << "\033[38;2;0;0;255;48;2;0;0;0m";
          }
          break;
        case LogToken::green:
          if (Log::useColors) {
            strm << "\033[38;2;0;255;0;48;2;0;0;0m";
          }
          break;
        case LogToken::yellow:
          if (Log::useColors) {
            strm << "\033[38;2;255;255;0;48;2;0;0;0m";
          }
          break;
        case LogToken::cyan:
          if (Log::useColors) {
            strm << "\033[38;2;0;255;255;48;2;0;0;0m";
          }
          break;
        case LogToken::off:
          if (Log::useColors) {
            strm << "\033[0m";
          }
          break;
        default:
          break;
    }
  }
} // dump
}

void
Log::endgroup()
{
  // Called at end of a LogInfo, etc. to write to log
  if ((Log::mode >= Log::myCurrentLevel) &&
    !Log::isPaused())
  {
    // Check our message filter here...
    // std::stringstream temp;
    // buffer.swap(temp);

    // Using BOOST logging setup
    // which could do things like log rotation, etc. at some point
    if (engine == 1) {
      logging::record rec = Log::mySevLog.open_record();
      if (rec) {
        logging::record_ostream booststream(rec);
        dump<logging::record_ostream>(booststream);
        Log::mySevLog.push_record(boost::move(rec));
      }

      // Using std::cout/std::err
      // which works better when using external log capturing
    } else {
      if (Log::mode == Log::Severity::SEVERE) {
        dump<std::ostream>(std::cerr);
      } else {
        dump<std::ostream>(std::cout);
      }
    }
  }
  // Always clear the message for next one
  // Not implemented in some older libc
  // std::stringstream temp;
  // buffer.swap(temp);
  buffer.clear();
  buffer.str("");
} // Log::endgroup

void
Log::BoostFormatter(logging::record_view const& rec, logging::formatting_ostream& strm)
{
  // We do all the work ourselves in Log just send to boost sink
  strm << rec[expr::smessage];
} // Log::BoostFormatter

void
Log::pauseLogging()
{
  ++myPausedLevel;
}

void
Log::restartLogging()
{
  --myPausedLevel;
}

bool
Log::isPaused()
{
  return (myPausedLevel > 0);
}

void
Log::flush()
{
  // Engine might change..so flush all of them
  instance()->mySink->locked_backend()->flush();
  std::cout << std::flush;
  std::cerr << std::flush;
}

Log::Log()
  : std::ostream(this), mySink(boost::make_shared<text_sink>())
{
  // -----------------------------------------------------
  // BOOST setup

  // We could do files and autorotation all with boost if wanted...
  // ..but we use our runner scripting
  // sink->locked_backend()->add_stream(
  //    boost::make_shared< std::ofstream >("sample.log"));

  // We have to provide an empty deleter to avoid destroying the global stream object
  boost::shared_ptr<std::ostream> stream(&std::clog, boost::log::v2_mt_posix::empty_deleter());

  mySink->locked_backend()->add_stream(stream);

  // We flush on a timer
  mySink->locked_backend()->auto_flush(false);
  mySink->set_formatter(&BoostFormatter);
  logging::core::get()->add_sink(mySink);
  // -----------------------------------------------------

  // We use our own.
  // logging::add_common_attributes();

  // INFO implement/ start up logging before configuration
  // We filter our own due to pause ability/etc.
  // logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::info);

  // Set fallback log pattern
  setLogPattern("[%TIME%] %MESSAGE%");

  // Set up a timer to auto flush logs, default for now until set by config
  auto flusher = make_shared<LogFlusher>(900);

  EventLoop::addEventHandler(flusher);
  myLogFlusher = flusher;
}

void
Log::setLogPattern(const std::string& pattern)
{
  std::vector<std::string> tokens =
  { "%TIME%",   "%TIMEMS%", "%MESSAGE%", "%FILE%", "%LINE%",   "%FUNCTION%",
    "%LEVEL%",
    "%ECOLOR%", "%RED%",    "%GREEN%",   "%BLUE%", "%YELLOW%", "%CYAN%",
    "%OFF%" };
  // Output
  std::vector<int> outputtokens;    // Token numbers in order
  std::vector<std::string> fillers; // Stuff between tokens

  Strings::TokenScan(pattern, tokens, outputtokens, fillers);

  // Thankfully we're not threaded, otherwise we'll need to start
  // adding locks for this stuff
  Log::fillers = fillers;
  // Log::outputtokens = outputtokens;

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
  bool found = false;

  Log::Severity severity = Log::Severity::SEVERE; // fall back

  std::vector<char> buf;

  if (IOURL::read(myURL, buf) > 0) {
    // LogSevere("Found log setting information at \"" << aURL << "\"\n");

    // For first attempt, we just find the first line of text that matches
    // a logging level and use it.  Granted we could modify lots of stuff
    buf.push_back('\0');
    std::istringstream is(&buf.front());
    std::string s;
    while (getline(is, s, '\n')) {
      Strings::trim(s);
      // LogSevere("Candidate string " << s << "\n");
      if (stringToSeverity(s, severity)) {
        // LogSevere("Found log setting from URL: " << severityToString(severity) << "\n");
        found = true;
        break;
      }
    }
  }
  if (!found) {
    LogSevere("Can't set log level from  \"" << myURL << "\"\n, using severe.\n");
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
    myCurrentLevel = severity;

    const bool enableStackTrace = (severity == Severity::DEBUG);
    const bool wantCoreDumps    = (severity == Severity::DEBUG);
    Signals::initialize(enableStackTrace, wantCoreDumps);
    firstTime = false;
  }
}

void
Log::setSeverityString(const std::string& level)
{
  auto& l = *(Log::instance());

  // Does the string match a known level...
  Severity severity = Severity::SEVERE;

  if (!stringToSeverity(level, severity)) {
    // .. if not try to use it as a URL...
    if (l.myLogURLWatcher == nullptr) {
      // Set up a timer to auto try the possible URL every so often.
      // FIXME: No way to configure timer at moment.  We need to balance
      // between hammering OS and taking too long to update.  For now change every 30 seconds
      std::shared_ptr<LogSettingURLWatcher> watcher = std::make_shared<LogSettingURLWatcher>(level, 30000);
      EventLoop::addEventHandler(watcher);
      l.myLogURLWatcher = watcher;
    } else {
      l.myLogURLWatcher->setURL(level);
    }
    l.myLogURLWatcher->action(); // Force watcher to update from file now
  } else {
    l.setSeverity(severity);
  }
}

void
Log::setLogEngine(const std::string& newengine)
{
  // We start up with the cout engine for messages 'before' log activated
  if (newengine == "boost") {
    engine = 1;
  } else {
    engine = 0;
  }
}

void
Log::setLogFlushMilliSeconds(int newflush)
{
  // Update flush timer
  auto& l = *(Log::instance());

  l.myLogFlusher->setTimerMilliseconds(newflush);
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
  auto& l = *(Log::instance());

  // Show current settings we're using
  std::string realLevel;

  if (l.myLogURLWatcher != nullptr) {
    realLevel = "From file:" + l.myLogURLWatcher->getURL().toString();
  } else {
    realLevel = severityToString(l.myCurrentLevel);
  }
  LogInfo("Current Logging Settings Level: " << realLevel << " "
                                             << "Flush: " << l.myLogFlusher->getTimerMilliseconds()
                                             << " milliseconds. \n");
}
