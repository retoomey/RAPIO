#pragma once

#include <rUtility.h>
#include <rURL.h>
#include <rEventTimer.h>

#include <memory>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/filesystem.hpp>

namespace rapio {
/** Enum class for log pattern tokens */
enum class LogToken {
  filler, time, timems, message, file, line, function, level, ecolor, red, green, blue, yellow, cyan, off
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
class Log : public Utility {
public:

  /** The standard format string date of form [date UTC] we use for logging. */
  static std::string LOG_TIMESTAMP;

  /** The standard format string date of form [date UTC] we use for logging with MS. */
  static std::string LOG_TIMESTAMP_MS;

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
    static std::shared_ptr<Log> mySingleton;

    if (mySingleton == 0) {
      // Not using make_shared here since Log() is private.
      mySingleton = std::shared_ptr<Log>(new Log());
    }
    return (mySingleton.get());
  }

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

  /** Set the log flush update time */
  static void
  setLogFlushMilliSeconds(int newflush);

  /** Set the log color output */
  static void
  setLogUseColors(bool flag);

  /** Set the log pattern */
  static void
  setLogPattern(const std::string& pattern);

  /** Print out the current logging settings being used */
  static void
  printCurrentLogSettings();

  /** Not virtual since you are not meant to derive from Log. */
  ~Log();

  /** Boost severity level, call this with macros only, public only because it has to be */
  static boost::log::sources::severity_logger<boost::log::trivial::severity_level> mySevLog;

  /** Color setting, public only because it has to be */
  static bool useColors;

  /** Parsed Token number list for logging */
  static std::vector<int> outputtokens;

  /** Parsed Token number list for logging */
  static std::vector<LogToken> outputtokensENUM;

  /** Parsed between token filler strings for logging */
  static std::vector<std::string> fillers;

private:

  /** The formatter callback for boost logging.
   * Warning do not call Log functions here, you will crash. Use std::cout */
  static void
  BoostFormatter(boost::log::record_view const& rec, boost::log::formatting_ostream& strm);

  /** The current verbosity level. */
  Severity myCurrentLevel;

  /** Are we myPausedLevel? This is an int, rather than a bool to allow
   *  nested calls ... */
  int myPausedLevel;

  /** Boost log sink */
  typedef boost::log::sinks::synchronous_sink<boost::log::sinks::text_ostream_backend> text_sink;
  boost::shared_ptr<text_sink> mySink;

  /** Current Log flush timer, if any */
  std::shared_ptr<LogFlusher> myLogFlusher;

  /** Current URL watcher, if any */
  std::shared_ptr<LogSettingURLWatcher> myLogURLWatcher;

  /** A singleton; not meant to be instantiated by anyone else. */
  Log();
}
;
}

#define MY_BOOST_LOGGER(sv) \
  BOOST_LOG_SEV(rapio::Log::mySevLog, sv) \
    << boost::log::add_value("Line", __LINE__)      \
    << boost::log::add_value("File", __FILE__)       \
    << boost::log::add_value("Function", BOOST_CURRENT_FUNCTION)

#define LogDebug(x)  MY_BOOST_LOGGER(boost::log::trivial::debug) << x
#define LogInfo(x)   MY_BOOST_LOGGER(boost::log::trivial::info) << x
#define LogSevere(x) MY_BOOST_LOGGER(boost::log::trivial::error) << x
