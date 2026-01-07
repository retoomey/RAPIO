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

struct LogArg {
  enum Type { Int, UInt, Long, ULong, Double, Bool, String, Pointer };
  Type type;

  union Data {
    long long          i;
    unsigned long long u;
    double             d;
    bool               b;
    const char *       s;
    const void *       p;
  } value;

  // Overloads for automatic type detection
  LogArg(int v)                : type(Int){ value.i = v; }

  LogArg(unsigned int v)       : type(UInt){ value.u = v; }

  LogArg(long long v)          : type(Long){ value.i = v; }

  LogArg(unsigned long long v) : type(ULong){ value.u = v; }

  LogArg(float v)              : type(Double){ value.d = static_cast<double>(v); }

  LogArg(double v)             : type(Double){ value.d = v; }

  LogArg(bool v)               : type(Bool){ value.b = v; }

  LogArg(const char * v)        : type(String){ value.s = v ? v : "(null)"; }

  LogArg(const std::string& v) : type(String){ value.s = v.c_str(); }

  LogArg(const void * v)        : type(Pointer){ value.p = v; }
};

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
  log(LogLevel level, const char * fmtStr, const std::vector<LogArg>& args) = 0;

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

    log(LogLevel::Debug, "{}", { LogArg(logline) });
  }

  /** Info - Single string overload for deprecated way */
  void
  infoOld(int line, const char * file, const char * function, const std::string& str)
  {
    // We pass "{}" as the format string and 'str' as the argument
    // Remove the "\n" if any. Current all the LogInfos are adding \n which is bad
    // practice.
    std::string logline = (str.back() == '\n') ? str.substr(0, str.length() - 1) : str;

    log(LogLevel::Info, "{}", { LogArg(logline) });
  }

  /** Info - Single string overload for deprecated way */
  void
  severeOld(int line, const char * file, const char * function, const std::string& str)
  {
    std::string logline = (str.back() == '\n') ? str.substr(0, str.length() - 1) : str;

    log(LogLevel::Error, "{}", { LogArg(logline) });
  }

  /** Debug */
  template <typename ... Args>
  void
  debug(int line, const char * file, const char * function, const char * fmtStr, Args&&... args)
  {
    log(LogLevel::Debug, fmtStr, { LogArg(std::forward<Args>(args))... });
  }

  /** Info */
  template <typename ... Args>
  void
  info(int line, const char * file, const char * function, const char * fmtStr, Args&&... args)
  {
    log(LogLevel::Info, fmtStr, { LogArg(std::forward<Args>(args))... });
  }

  /** Severe.  (We used severe forever, match to error) */
  template <typename ... Args>
  void
  severe(int line, const char * file, const char * function, const char * fmtStr, Args&&... args)
  {
    log(LogLevel::Error, fmtStr, { LogArg(std::forward<Args>(args))... });
  }
};
}
