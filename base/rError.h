#pragma once

#include <rUtility.h>

#include <iosfwd>
#include <sstream> // for ostringstream
#include <map>
#include <memory>

namespace rapio {
/**
 * A singleton error logger class. This class will be used by all libraries.
 * Typical usage of this class is as follows:
 * <pre>
 * LogSevere( "Error: " << file_name << " not readable.\n");
 * </pre>
 * @see Error
 */
class Log : public Utility {
public:

  /** returns the singleton instance. */
  inline static Log *
  instance()
  {
    static std::shared_ptr<Log> mySingleton;

    if (mySingleton == 0) {
      mySingleton = std::shared_ptr<Log>(new Log());
    }
    return (mySingleton.get());
  }

  enum Severity {
    /** Debugging Messages, high volume */
    Debug          = 1,

    /** Just for information. */
    Info           = 10,

    /** Information that provides program flow */
    ImpInfo        = 11,

    /** Environment problem, can slide. */
    NotSevere      = 20,

    /** Software Bug, needs programmer attention. */
    NotAnticipated = 21,

    /** Environment problem, needs user attention. */
    Severe         = 22,

    /** Software Bug, needs programmer attention. */
    Logical        = 23,
  };

  /**
   *  Streams to log to.
   *  This class allows multiple references to the same stream,
   *  removing a stream only when the last reference to it vanishes.
   */
  class Streams : public Utility {
    typedef std::map<std::ostream *, int> StreamSet;
    StreamSet myStreams;

private:

    friend class Log;

    /** The default constructor, private to permit use only by Log */
    Streams();

    /** Add a stream. Returns the number of references to it
     *  after the addition. */
    size_t
    addStream(std::ostream * os);

    /** remove a stream. Returns the number of references to it
     *  after the deletion. */
    size_t
    removeStream(std::ostream * os);

    /** cycle any stream if the cycle size has been exceeded. */
    void
    cycle(size_t myCycleSize);

public:

    /** flush any stream. */
    void
    flush();

public:

    template <class T>
    Streams&
    operator << (const T& obj)
    {
      for (StreamSet::iterator iter = myStreams.begin();
        iter != myStreams.end(); ++iter) { (*(iter->first)) << obj; }
      return (*this);
    }

    Streams&
    operator << (std::ostream& eg_endl)
    {
      for (StreamSet::iterator iter = myStreams.begin();
        iter != myStreams.end(); ++iter) { (*(iter->first)) << eg_endl.rdbuf(); }
      return (*this);
    }
  }
  ;

  /**
   * The logger starts logging to the passed in stream.
   * @param s stream to log to.
   */
  static void
  startLogging(std::ostream * s);

  /**
   * The logger stops logging to the passed in stream if there are no
   * other references to this stream.
   */
  static void
  stopLogging(std::ostream * s);

  /**
   * Pause logging. This is useful when calling a function that you know
   * could cause an error.  Remember to call restartLogging ...
   *
   * Nested calls to pause and restart are allowed.
   */
  static void
  pauseLogging();

  /**
   * Pause logging. This is useful when calling a function that you know
   * could cause an error ...  You might want to call restartLogging ...
   */
  static void
  restartLogging();

  /** Flush the log stream */
  static void
  flush();

  /**
   * Returns the current stream that the end-user of the library has specified.
   * @param req_level the severity level of the message(s) that you are
   * going to log to this stream.
   */
  static Streams&
  get(Severity req_level);

public:

  /** Change the severity threshold. By default, it is at ImpInfo */
  static void
  setSeverityThreshold(Severity new_level)
  {
    instance()->myCurrentLevel = new_level;
  }

  /** Is above level? */
  inline static bool
  shouldLog(Severity desired_level)
  {
    return (desired_level >= instance()->myCurrentLevel);
  }

  static void
  setVerbose(const std::string& in);

  /** Set the log settings for application.  Typically called once on startup */
  static void
  setLogSettings(const std::string& level,
    int                           flush,
    int                           logSize);

  /**
   * Set a size beyond which the log file should not grow. The log
   * file will be cycled through at this point.
   */
  static void
  setLogSize(int newSize);

  /** Not virtual since you are not meant to derive from Log. */
  ~Log();

private:

  /** The current verbosity level. */
  Severity myCurrentLevel;

  /** /dev/null stream ... */
  Streams myNullStream;

  /** regular stream ... */
  Streams myLogStream;

  /** Are we myPausedLevel? This is an int, rather than a bool to allow
   *  nested calls ... */
  int myPausedLevel;

  /** What is the maximum log file size? We'll cycle at this time. */
  int myCycleSize;

  /** A singleton; not meant to be instantiated by anyone else. */
  Log();
}
;

/** A single error message is denoted by Error.  */
class Error : public Utility {
public:

  /**
   * Starts logging here on out to the stream passed in.
   * Usually, you would want to log to a strstream.
   * @param s  the stream to log into.
   */
  Error(std::ostream * s) : err_stream(s)
  {
    Log::startLogging(err_stream);
  }

  /**
   * Starts logging here on out to the stream passed in.
   * Usually, you would want to log to a strstream.
   * @param s  the stream to log into.
   */
  Error(std::ostream& s) : err_stream(&s)
  {
    Log::startLogging(err_stream);
  }

  /**
   * From the moment of construction to the point of destruction
   * defines a single message. All errors by CODE components in the
   * meantime are shoved in the stream passed in.
   */
  ~Error()
  {
    Log::stopLogging(err_stream);
  }

protected:

  std::ostream * err_stream;
}
;
}

/* define __func__ for non-ANSI compilers... */
// #if __STDC_VERSION__ < 199901L
// # if __GNUC__ >= 2
// #  define __func__ __FUNCTION__
// # else // if __GNUC__ >= 2
// #  define __func__ "<unknown>"
// # endif // if __GNUC__ >= 2
// #endif // if __STDC_VERSION__ < 199901L
// #define LINE_ID __FILE__ << ':' << __LINE__ << ' ' << __func__
#define LINE_ID __FILE__ << ':' << __LINE__

/**
 * Macro for calling Log at a given severity level
 * \code
 * LogInfo( x << " " << y );
 * \endcode
 */
#define LogAtSeverity(severity, x)                                             \
  do {                                                                         \
    if (rapio::Log::shouldLog(severity)) { rapio::Log::get(severity) << '('    \
                                                                     << LINE_ID << ") " << x; }             \
  } while (0) /* (no trailing ; ) */

#define LogDebug(x)   LogAtSeverity(rapio::Log::Debug, x)
#define LogInfo(x)    LogAtSeverity(rapio::Log::Info, x)
#define LogImpInfo(x) LogAtSeverity(rapio::Log::ImpInfo, x)
#define LogSevere(x)  LogAtSeverity(rapio::Log::Severe, x)
