#pragma once

#include <rIO.h>

#include <string>
#include <vector>
#include <memory>
#include <utility>

#include <iostream>

// Goal is for this to work
#include <fmt/format.h>

namespace rapio {
/** Enum class for log pattern tokens */
enum class LogToken {
  filler, time, timems, timec, message, file, line, function, level, ecolor, red, green, blue, yellow, cyan, off,
  threadid
};

// These actual match the spd logger levels.  We 'could' expand ability
// to add more levels.  Right now we've been keeping it simple.
enum class LogLevel { Trace, Debug, Info, Warn, Error, Critical, Off };

/**
 * Logger has the ability to write log messages.
 * Making it dynamic module so we can swap different logging libraries or
 * techniques depending on need.  For example, maybe I'll log directly
 * to syslog, or use boost, or use AWS logging, or use spdlog, etc.
 *
 * @author Robert Toomey
 */
class Logger : public IO {
public:

  virtual
  ~Logger() = default;

  // The Bridge: Implemented by the DLL
  virtual void
  log(LogLevel level, const std::string& message) = 0;

  template <typename ... Args>
  void
  logFormatted(LogLevel level, fmt::format_string<Args...> fmtStr, Args&&... args)
  {
    fmt::memory_buffer buf;

    #if __cplusplus >= 202002L
    // C++20: The compiler validates fmtStr against Args... at the call site.
    // No try/catch needed for the format string itself.
    fmt::format_to(std::back_inserter(buf), fmtStr, std::forward<Args>(args)...);
    #else
    // Pre-C++20: fmt::format_string doesn't trigger compile errors.
    // We MUST use fmt::runtime and a try/catch to prevent crashes.
    try {
      fmt::format_to(std::back_inserter(buf), fmt::runtime(fmtStr), std::forward<Args>(args)...);
    } catch (const fmt::format_error& e) {
      // If the format is bad, log the error so the dev can fix the code
      log(LogLevel::Error, std::string("Format Error: ") + e.what());
      return;
    }
    #endif // if __cplusplus >= 202002L

    log(level, fmt::to_string(buf));
  }

  /** Notify logger of token pattern change */
  virtual void
  setTokenPattern(const std::vector<LogToken>& tokens) = 0;

  /** Notify logger of logging level change.  Going to use
   * the 'expanded' logging modes. */
  virtual void
  setLoggingLevel(LogLevel l) = 0;

  /** Notify logger we're wanting to flush immediately */
  virtual void
  flush() = 0;

  /** Notify logger as to desired flush interval in milliseconds */
  virtual void
  setFlushMilliseconds(int ms) = 0;

  /** Info - Single string overload for deprecated way */
  void
  debugOld(int line, const char * file, const char * function, const std::string& str)
  {
    std::string logline = (str.back() == '\n') ? str.substr(0, str.length() - 1) : str;

    log(LogLevel::Debug, logline);
  }

  /** Info - Single string overload for deprecated way */
  void
  infoOld(int line, const char * file, const char * function, const std::string& str)
  {
    // We pass "{}" as the format string and 'str' as the argument
    // Remove the "\n" if any. Current all the LogInfos are adding \n which is bad
    // practice.
    std::string logline = (str.back() == '\n') ? str.substr(0, str.length() - 1) : str;

    log(LogLevel::Info, logline);
  }

  /** Info - Single string overload for deprecated way */
  void
  severeOld(int line, const char * file, const char * function, const std::string& str)
  {
    std::string logline = (str.back() == '\n') ? str.substr(0, str.length() - 1) : str;

    log(LogLevel::Error, logline);
  }

  /** Debug */
  template <typename ... Args>
  void
  debug(int line, const char * file, const char * function,
    fmt::format_string<Args...> fmtStr, Args&&... args)
  {
    logFormatted(LogLevel::Debug, fmtStr, std::forward<Args>(args)...);
  }

  /** Info */
  template <typename ... Args>
  void
  info(int line, const char * file, const char * function,
    fmt::format_string<Args...> fmtStr, Args&&... args)
  {
    logFormatted(LogLevel::Info, fmtStr, std::forward<Args>(args)...);
  }

  /** Severe.  (We used severe forever, match to error) */
  template <typename ... Args>
  void
  severe(int line, const char * file, const char * function,
    fmt::format_string<Args...> fmtStr, Args&&... args)
  {
    logFormatted(LogLevel::Error, fmtStr, std::forward<Args>(args)...);
  }
};
}
