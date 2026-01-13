#include "rLogSPD.h"
#include "rColorTerm.h"
#include "rError.h"
#include "rTime.h"

using namespace rapio;

#include "spdlog/spdlog.h"
#include "spdlog/pattern_formatter.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/fmt/bundled/format.h" // spdlog's internal fmt
#include "spdlog/fmt/bundled/args.h"   // For dynamic_format_arg_store

std::shared_ptr<spdlog::logger> LogSPD::mySpdLogger = nullptr;

constexpr char SPD1 = '*';
constexpr char SPD2 = '~';
constexpr char SPD3 = '&';

/** A custom formatting for our various time format strings,
 * in particular we prefer utc times */
class fmtTime : public spdlog::custom_flag_formatter
{
public:

  fmtTime(const std::string& rapioFormat) : myFormat(rapioFormat){ }

  void
  format(const spdlog::details::log_msg &msg, const std::tm &a, spdlog::memory_buf_t &dest) override
  {
    const auto time      = Time(msg.time);
    std::string some_txt = time.getString(myFormat);

    dest.append(some_txt.data(), some_txt.data() + some_txt.size());
  }

  std::unique_ptr<custom_flag_formatter>
  clone() const override
  {
    return spdlog::details::make_unique<fmtTime>(myFormat);
  }

private:
  std::string myFormat;
};

// Library dynamic link to create this factory
extern "C"
{
void *
createRAPIOLOG(void)
{
  auto * z = new LogSPD();

  z->initialize();

  return reinterpret_cast<void *>(z);
}
};

namespace {
inline spdlog::level::level_enum
map_level(LogLevel level)
{
  switch (level) {
      case LogLevel::Trace:    return spdlog::level::trace;

      case LogLevel::Debug:    return spdlog::level::debug;

      case LogLevel::Warn:     return spdlog::level::warn;

      case LogLevel::Error:    return spdlog::level::err;

      case LogLevel::Critical: return spdlog::level::critical;

      case LogLevel::Off: return spdlog::level::off;

      default:                 return spdlog::level::info;
  }
}
}

void
LogSPD::initialize()
{
  // Immediate create logger
  mySpdLogger = spdlog::stdout_color_mt("RAPIO2");
}

void
LogSPD::setTokenPattern(const std::vector<LogToken>& tokens)
{
  // We could embrace spdlog 100% and
  // use its formatters in our log settings. For
  // now we'll translate to spd here
  std::ostringstream strm;

  // Logging is configurable by pattern string
  size_t fat = 0;

  for (auto a:tokens) {
    switch (a) {
        case LogToken::filler:
          strm << Log::fillers[fat++];
          break;
        case LogToken::time:   // Current UTC time
          strm << "%" << SPD1; // Log::LOG_TIMESTAMP);
          break;
        case LogToken::timems: // Current UTC time with milliseconds
          strm << "%" << SPD2; // Log::LOG_TIMESTAMP_MS);
          break;
        case LogToken::timec:  // Current clock with milliseconds
          strm << "%" << SPD3; // Log::LOG_TIMESTAMP_C);
          break;
        case LogToken::message:
          strm << "%v";
          break;
        case LogToken::file:
          strm << "%s";
          break;
        case LogToken::line:
          strm << "%#";
          break;
        case LogToken::function:
          strm << "%!";
          break;
        case LogToken::level:
          strm << "%l";
          break;
        case LogToken::ecolor:
          strm << "%^";
          break;
        case LogToken::red:
          strm << ColorTerm::red();
          break;
        case LogToken::blue:
          strm << ColorTerm::blue();
          break;
        case LogToken::green:
          strm << ColorTerm::green();
          break;
        case LogToken::yellow:
          strm << ColorTerm::yellow();
          break;
        case LogToken::cyan:
          strm << ColorTerm::cyan();
          break;
        case LogToken::off:
          strm << "%$";
          strm << ColorTerm::reset();
          break;
        case LogToken::threadid:
          strm << "%t";
          break;
        default:
          break;
    }
  } // loop
  myPattern = strm.str();

  setSPDPattern(myPattern);
} // LogSPD::setTokenPattern

void
LogSPD::setLoggingLevel(LogLevel level)
{
  spdlog::level::level_enum spd_level = map_level(level);

  mySpdLogger->set_level(spd_level);
}

void
LogSPD::setSPDPattern(const std::string& pattern)
{
  auto formatter = std::unique_ptr<spdlog::pattern_formatter>(new spdlog::pattern_formatter());

  // We have a bunch of UTC formats we support
  formatter->add_flag<fmtTime>(SPD1, Log::LOG_TIMESTAMP);
  formatter->add_flag<fmtTime>(SPD2, Log::LOG_TIMESTAMP_MS);
  formatter->add_flag<fmtTime>(SPD3, Log::LOG_TIMESTAMP_C);
  formatter->set_pattern(myPattern);
  mySpdLogger->set_formatter(std::move(formatter));
  // mySpdLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");
}

void
LogSPD::flush()
{
  mySpdLogger->flush();
}

void
LogSPD::setFlushMilliseconds(int ms)
{
  // This is global to all spdlogs.
  spdlog::flush_every(std::chrono::milliseconds(ms));
}

void
LogSPD::log(LogLevel level, const std::string& message)
{
  // 1. Map custom LogLevel to spdlog level
  spdlog::level::level_enum spd_level = map_level(level);

  // 2. Log to spd Map custom LogLevel to spdlog level
  mySpdLogger->log(spd_level, "{}", message);
} // LogSPD::log
