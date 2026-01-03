#pragma once

#include <rLogger.h>

#include <string>
#include <vector>
#include <memory>
// #include <utility>

namespace spdlog {
class logger;
}

namespace rapio {
/**
 * Spd logger implementation.
 * I'm liking this logger, especially as we move to more threaded
 * algorithms.  As a module we can fall back to other logging or
 * experiment with different logging if needed.
 *
 * This requires the fmt library and spdlog, which isn't 100% available
 * on every linux distro we support, but we're building it private
 * in this module.
 *
 * @author Robert Toomey
 */
class LogSPD : public Logger {
public:
  /** Log a message at given level */
  void
  log(LogLevel level, const char * fmtStr, const std::vector<LogArg>& args);

  /** Set the logging tokens for the log line pattern */
  void
  setTokenPattern(const std::vector<LogToken>& tokens);

  /** Set the logging level cutoff */
  void
  setLoggingLevel(LogLevel l);

  /** Flush the log */
  void
  flush();

  /** Set log flush milliseconds */
  void
  setFlushMilliseconds(int ms);

  /** Initialize and set up logger */
  void
  initialize();

private:

  /** Set the current spd log pattern */
  void
  setSPDPattern(const std::string& pattern);

  /** The actual spd logger object */
  static std::shared_ptr<spdlog::logger> mySpdLogger;

  /** Current spd log pattern */
  std::string myPattern;
};
}
