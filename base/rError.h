#pragma once

#include <stdexcept>
#include <string>
#include <cstring> // for strerror

namespace rapio {
/** Base exception for all RAPIO framework errors */
class RAPIOException : public std::runtime_error {
public:
  explicit RAPIOException(const std::string& msg) : std::runtime_error(msg){ }
};

/** Specific exception for initialization, configuration, and option parsing failures */
class StartupException : public RAPIOException {
public:
  explicit StartupException(const std::string& msg) : RAPIOException(msg){ }
};

/** Exception wrapper for any C function calls we make that return standard 0 or error code.
 * Call using the ERRNO(function) ability */
class ErrnoException : public std::exception
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
}

#define ERRNO(e) \
  { errno = 0; { e; } if ((errno != 0)) { throw ErrnoException(errno, # e); } }

// Hack to avoid changing all files including this
// We could make it slightly cleaner having everything mostly include logger
// but this works for now.  They pretty much go together 99% of the time
#include <rLogger.h>
