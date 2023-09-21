#pragma once

#include <rUtility.h>
#include <rURL.h>
#include <rEventTimer.h>

#include <memory>
#include <mutex>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/filesystem.hpp>

typedef boost::log::sinks::synchronous_sink<boost::log::sinks::text_ostream_backend> text_sink;

namespace rapio {
/** Enum class for log pattern tokens */
enum class LogToken {
  filler, time, timems, timec, message, file, line, function, level, ecolor, red, green, blue, yellow, cyan, off,
  threadid
};

/** Flushes the log on a timer in main event loop */
class LogFlusher : public EventTimer {
public:

  LogFlusher(size_t milliseconds) : EventTimer(milliseconds, "Log flusher")
  { }

  /** Fire the action of checking/flushing the log */
  virtual void
  action() override;
};

/** Watches a setting URL for changes and updates log severity on the fly */
class LogSettingURLWatcher : public EventTimer {
public:

  LogSettingURLWatcher(const URL& aURL, size_t milliseconds) : EventTimer(milliseconds, "Log Setting URL Watcher"),
    myURL(aURL)
  { }

  /** Get the URL we're watching */
  URL
  getURL(){ return myURL; }

  /** Set the URL we're watching */
  void
  setURL(const URL& aURL){ myURL = aURL; }

  /** Fire the action of checking/updating log settings */
  virtual void
  action() override;

protected:

  /** The URL that we are watching and getting log settings from */
  URL myURL;
};

/**
 * A singleton error logger class. This class will be used by all libraries.
 * Typical usage of this class is as follows:
 * <pre>
 * LogSevere( "Error: " << file_name << " not readable.\n");
 * </pre>
 */
class Log : public std::ostream, std::streambuf, Utility {
public:

  /** Severity levels in decreasing order of printing */
  enum class Severity {
    /** Debugging Messages, high volume */
    DEBUG  = 1,

    /** Just for information. */
    INFO   = 10,

    /** Major problem */
    SEVERE = 20
  };

  /** Returns the singleton instance. */
  inline static Log *
  instance()
  {
    if (mySingleton == 0) {
      // Not using make_shared here since Log() is private.
      mySingleton = std::shared_ptr<Log>(new Log());
    }
    return (mySingleton.get());
  }

  /** Lock for critical log settings/printing */
  static std::mutex logLock;

  /** Current log group severity mode */
  // static Log::Severity mode;

  /** The standard format string date of form [date UTC] we use for logging. */
  static std::string LOG_TIMESTAMP;

  /** The standard format string date of form [date UTC] we use for logging with MS. */
  static std::string LOG_TIMESTAMP_MS;

  /** The standard format string date of form [date UTC] we use for logging just clock. */
  static std::string LOG_TIMESTAMP_C;

  /** Log singleton */
  static std::shared_ptr<Log> mySingleton;

  /**
   * Pause logging. This is useful when calling a function that you know
   * could cause an error.  Remember to call restartLogging ...
   *
   * Nested calls to pause and restart are allowed.
   */
  static void
  pauseLogging();

  /**
   * Restart logging. This undoes a single pauseLogging call
   */
  static void
  restartLogging();

  /** Is logging paused? */
  static bool
  isPaused();

  /** Flush the log stream */
  static void
  flush();

  /** Set the log severity level */
  static void
  setSeverity(Log::Severity severity);

  /** Set the log severity level from a string which can be a level or a URL */
  static void
  setSeverityString(const std::string& level);

  /** Set the log color output */
  static void
  setLogEngine(const std::string& engine);

  /** Set the log flush update time */
  static void
  setLogFlushMilliSeconds(int newflush);

  /** Set the log color output */
  static void
  setLogUseColors(bool flag);

  /** Set the help color output */
  static void
  setHelpColors(bool flag);

  /** Set the log pattern */
  static void
  setLogPattern(const std::string& pattern);

  /** Print out the current logging settings being used */
  static void
  printCurrentLogSettings();

  /** Boost severity level, call this with macros only, public only because it has to be */
  static boost::log::sources::severity_logger<boost::log::trivial::severity_level> mySevLog;

  /** Color setting for basic log entries */
  static bool useColors;

  /** Color setting for help */
  static bool useHelpColors;

  /** Parsed Token number list for logging */
  static std::vector<int> outputtokens;

  /** Parsed Token number list for logging */
  static std::vector<LogToken> outputtokensENUM;

  /** Parsed between token filler strings for logging */
  static std::vector<std::string> fillers;

  /** The current verbosity level. */
  static Severity myCurrentLevel;

  /** Are we myPausedLevel? This is an int, rather than a bool to allow
   *  nested calls ... */
  static int myPausedLevel;

  /** Simple number for engine for moment, could be enum */
  static int engine;

  /** Not virtual since you are not meant to derive from Log. */
  ~Log();

private:

  /** The formatter callback for boost logging.
   * Warning do not call Log functions here, you will crash. Use std::cout */
  static void
  BoostFormatter(boost::log::record_view const& rec, boost::log::formatting_ostream& strm);

  // FIXME: Note, these variables are why we create a log object vs
  // having everything static.  Maybe routines to do it indirectly?

  /** Boost log sink */
  boost::shared_ptr<text_sink> mySink;

  /** Current Log flush timer, if any */
  std::shared_ptr<LogFlusher> myLogFlusher;

  /** Current URL watcher, if any */
  std::shared_ptr<LogSettingURLWatcher> myLogURLWatcher;

  /** A singleton; not meant to be instantiated by anyone else. */
  Log();
};

/** Log state object per LogInfo/etc. call.  This allows proper locking using RAII
 * even for exception cases */
class LogCall1 {
public:

  /** Create a log call with info for one LogInfo, etc. line of output */
  LogCall1(Log::Severity m, int l, const std::string& f, const std::string& f2)
    : mode(m), line(l), file(f), function(f2)
  {
    // Not locking on this, whatever we get is fine
    goodlog = ((mode >= Log::myCurrentLevel) && !Log::isPaused());
  }

  /** Final call to dump current buffer (thread safe) */
  void
  dump();

  // Variables we need to maintain by call for thread safety

  /** Good log command test */
  bool goodlog;
  /** Buffer of this log call */
  std::stringstream buffer;
  /** Set a log severity mode */
  Log::Severity mode;
  /** Set a log line number */
  int line;
  /** Set a log file name */
  std::string file;
  /** Set a log function inside name */
  std::string function;
};

/** Templates to allow our custom log wrapper */
template <class T> rapio::LogCall1&
operator << (rapio::LogCall1& l, const T& x)
{
  l.buffer << x; // Our per thread buffer per log call
  return l;
}
}

#define LogDebug(x) \
  { auto _aTempLog = rapio::LogCall1(rapio::Log::Severity::DEBUG, __LINE__, __FILE__, \
        BOOST_CURRENT_FUNCTION);  _aTempLog << x; \
    _aTempLog.dump(); }
#define LogInfo(x) \
  { auto _aTempLog = rapio::LogCall1(rapio::Log::Severity::INFO, __LINE__, __FILE__, \
        BOOST_CURRENT_FUNCTION);  _aTempLog << x; \
    _aTempLog.dump(); }
#define LogSevere(x) \
  { auto _aTempLog = rapio::LogCall1(rapio::Log::Severity::SEVERE, __LINE__, __FILE__, \
        BOOST_CURRENT_FUNCTION);  _aTempLog << x; \
    _aTempLog.dump(); }
