#pragma once

#include <rUtility.h>
#include <rURL.h>
#include <rEventTimer.h>
#include <rLogger.h>

#include <memory>
#include <sstream>

#include <string.h> // errno strs

#include <fmt/format.h>

namespace rapio {
/* Exception wrapper for any C function calls we make that return standard 0 or error code.
 * Call using the ERRNO(function) ability */
class ErrnoException : public Utility, public std::exception
{
public:
  ErrnoException(int err, const std::string& c) : std::exception(), retval(err),
    command(c)
  { }

  virtual ~ErrnoException() throw() { }

  int
  getErrnoVal() const
  {
    return (retval);
  }

  std::string
  getErrnoCommand() const
  {
    return (command);
  }

  std::string
  getErrnoStr() const
  {
    return (std::string(strerror(retval)) + " on '" + command + "'");
  }

private:

  /** The Errno value from the command (copied from errno) */
  int retval;

  /** The Errno command we wrapped */
  std::string command;
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
 * Logger class. This class will be used by all libraries.
 * Typical usage of this class is as follows:
 * <pre>
 *
 * fLogInfo("Hello {} and one {}", "World", 1);
 * fLogInfo("Pi is roughly {:.2f}", 3.14159);
 * void* some_ptr = (void*)(12345);
 * fLogInfo("Pointer value: {}", some_ptr);
 * fLogInfo("Boolean test: {} {}", true, false);
 * fLogInfo("User {} logged in from {} with ID {}", "Alice", "192.168.1.1", 42);
 * fLogInfo("{} + {} = {}", 10, 20, 10 + 20);
 * fLogInfo("Coordinates: x={} y={} z={}", 1.0, 2.5, -0.3);
 * fLogInfo("Name: {} Age: {} Score: {:.8f} Active: {}", "Bob", 30, 99.5, true);
 * std::string name = "Charlie";
 * const char* city = "Denver";
 * fLogInfo("Name: {} City: {}", name, city);
 * fLogInfo("Hex: {:#x} Oct: {:#o} Bin: {:#b}", 255, 255, 255);
 * fLogInfo("Padded number: {:05}", 42);
 * fLogInfo("Left align: {:<10} Right align: {:>10}", "left", "right");
 *
 * Old way of streaming is still supported at moment but not recommended.
 * fmt/python style of logging is faster
 *
 * LogSevere( "Error: " << file_name << " not readable.\n");
 * </pre>
 */
class Log : public Utility {
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

  /** Initialize logging */
  static void
  initialize();

  /** The standard format string date of form [date UTC] we use for logging. */
  static constexpr const char * LOG_TIMESTAMP = "%Y %m/%d %H:%M:%S UTC";

  /** The standard format string date of form [date UTC] we use for logging with MS. */
  static constexpr const char * LOG_TIMESTAMP_MS = "%Y %m/%d %H:%M:%S.%/ms UTC";

  /** The standard format string date of form [date UTC] we use for logging just clock. */
  static constexpr const char * LOG_TIMESTAMP_C = "%H:%M:%S.%/ms";

  /** The dynamic logger module doing the work */
  static std::shared_ptr<Logger> myLog;

  /** Flag for allowing debug (mode and pausing applied) */
  static bool wouldDebug;

  /** Flag for allowing info (mode and pausing applied) */
  static bool wouldInfo;

  /** Flag for allowing severe (mode and pausing applied) */
  static bool wouldSevere;

  /** Sync logging flags */
  static void
  syncFlags();

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

public:
  /** Color setting for basic log entries */
  static bool useColors;

  /** Parsed between token filler strings for logging */
  static std::vector<std::string> fillers;

private:

  /** Color setting for help */
  static bool useHelpColors;

  /** Parsed Token number list for logging */
  static std::vector<int> outputtokens;

  /** Parsed Token number list for logging */
  static std::vector<LogToken> outputtokensENUM;

  /** The current verbosity level. */
  static Severity myCurrentLevel;

  /** Are we myPausedLevel? This is an int, rather than a bool to allow
   *  nested calls ... */
  static int myPausedLevel;

  /** Simple number for engine for moment, could be enum */
  static int engine;

  /** Keep last set flush milliseconds for reference */
  static int myFlushMS;

  /** Current URL watcher, if any */
  static std::shared_ptr<LogSettingURLWatcher> myLogURLWatcher;
};
}

// LINE_ID macro in order to simplify debugging.
#define LINE_ID __FILE__ << ':' << __LINE__ << ' ' << __func__

/** Would this log at this level?  Useful for turning off calculations,
 * etc. that only output at a given log level */
#define wouldLogDebug()  rapio::Log::wouldDebug
#define wouldLogInfo()   rapio::Log::wouldInfo
#define wouldLogSevere() rapio::Log::wouldSevere

// -----------------------------------------------------------------------
// @Deprecated.  Using stringstream which is relatively slow.
// Replace logging with the new calls using format
#define LogDebug(x) \
  do { \
    if (rapio::Log::wouldDebug) { \
      std::stringstream _aTempLog; \
      _aTempLog << x; \
      rapio::Log::myLog->debugOld(__LINE__, __FILE__, __func__, _aTempLog.str()); \
    } \
  } while (0)
#define LogInfo(x) \
  do { \
    if (rapio::Log::wouldInfo) { \
      std::stringstream _aTempLog; \
      _aTempLog << x; \
      rapio::Log::myLog->infoOld(__LINE__, __FILE__, __func__, _aTempLog.str()); \
    } \
  } while (0)
#define LogSevere(x) \
  do { \
    if (rapio::Log::wouldSevere) { \
      std::stringstream _aTempLog; \
      _aTempLog << x; \
      rapio::Log::myLog->severeOld(__LINE__, __FILE__, __func__, _aTempLog.str()); \
    } \
  } while (0)
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// New calls using fmt which is faster

#define fLogDebug(fmtstr, ...) \
  do { \
    if (rapio::Log::wouldDebug) { \
      rapio::Log::myLog->debug(__LINE__, __FILE__, __func__, FMT_STRING(fmtstr), ## __VA_ARGS__); \
    } \
  } while (0)
#define fLogInfo(fmtstr, ...) \
  do { \
    if (rapio::Log::wouldInfo) { \
      rapio::Log::myLog->info(__LINE__, __FILE__, __func__, FMT_STRING(fmtstr), ## __VA_ARGS__); \
    } \
  } while (0)
#define fLogSevere(fmtstr, ...) \
  do { \
    if (rapio::Log::wouldSevere) { \
      rapio::Log::myLog->severe(__LINE__, __FILE__, __func__, FMT_STRING(fmtstr), ## __VA_ARGS__); \
    } \
  } while (0)

#define ERRNO(e) \
  { errno = 0; { e; } if ((errno != 0)) { throw ErrnoException(errno, # e); } }
