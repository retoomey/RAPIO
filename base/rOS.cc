#include "rOS.h"

#include "rError.h"

#include <cstring> // memset()
#include <cerrno>  // errno, EEXIST
#include <string>
#include <fstream>
#include <iostream>

#include <sys/stat.h>
#include <unistd.h>

#include <sys/utsname.h> // uname

#include "rProcessTimer.h"

using namespace rapio;

#ifndef PATH_MAX
# define PATH_MAX 4096
#endif // ifndef PATH_MAX

const std::string&
OS::getHostName()
{
  static bool first = true;
  static std::string hostname;

  if (first) {
    struct utsname name2;
    if (uname(&name2) == -1) {
      LogSevere("Uname lookup failure for localhost.\n");
      hostname = "localhost";
    } else {
      hostname = std::string(name2.nodename);
    }
    first = false;
  }
  return hostname;
}

std::string
OS::getProcessName()
{
  std::string name;
  char buf[PATH_MAX];
  memset(buf, 0, sizeof(buf));

  struct stat sbuf;

  if ((::stat("/proc/self/exe", &sbuf) == 0) &&
    ::readlink("/proc/self/exe", buf, sizeof(buf) - 1))
  {
    buf[sizeof(buf) - 1] = '\0';
    name = buf;
  }
  return (name);
}

namespace {
int
my_mkdir(const char * path)
{
  int retval;

  retval = mkdir(path, 0777);

  if (errno == EEXIST) {
    retval = 0;
  }
  LogDebug("my_mkdir [" << path << "] returns [" << retval << "]\n");
  return (retval);
}
}

std::string
OS::getCurrentDirectory()
{
  /*char buf[PATH_MAX];
   *
   * memset(buf, 0, sizeof(buf));
   * getcwd(buf, sizeof(buf));
   * return (buf);
   */
  // Get the current working directory
  std::string PWD = "/";
  char cwd[PATH_MAX];

  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    PWD = std::string(cwd);
  }
  return (PWD);
}

const unsigned int OS::FILE_IS_REGULAR(1 << 0);
const unsigned int OS::FILE_IS_SYMLINK(1 << 1);
const unsigned int OS::FILE_IS_DIRECTORY(1 << 2);
const unsigned int OS::FILE_IS_EXECUTABLE(1 << 3);
const unsigned int OS::FILE_EXIST(1 << 4);

unsigned int
OS::testFile(const std::string& file, unsigned int test)
{
  unsigned int result = 0;

  const bool exists(access(file.c_str(), F_OK) == 0);

  if (test & FILE_EXIST) {
    if (exists) {
      result |= FILE_EXIST;
    }
    test &= ~FILE_EXIST;
  }

  if ((test & FILE_IS_EXECUTABLE) && (access(file.c_str(), X_OK) == 0)) {
    if (getuid() != 0) {
      result |= FILE_IS_EXECUTABLE;
      test   &= ~FILE_IS_EXECUTABLE;
    }

    // For root, on some POSIX systems, access (file.c_str(), X_OK)
    // will succeed even if no executable bits are set on the
    // file. We fall through to a stat test to avoid that.
  } else {
    test &= ~FILE_IS_EXECUTABLE;
  }

  if (test & FILE_IS_SYMLINK) {
    struct stat s;
    const bool symlink(!lstat(file.c_str(), &s) && S_ISLNK(s.st_mode));

    if (symlink) {
      result |= FILE_IS_SYMLINK;
    }
    test &= ~FILE_IS_SYMLINK;
  }

  if (test & (FILE_IS_REGULAR
    | FILE_IS_DIRECTORY
    | FILE_IS_EXECUTABLE))
  {
    // In case it's a directory with a slash at the end.
    // stat won't work on windows unless we remove the slash.
    std::string tfile(file);

    if ((tfile.size() > 0) && (tfile[tfile.size() - 1] == '/')) {
      tfile = tfile.substr(0, tfile.size() - 1);
    }

    struct stat s;

    if (stat(tfile.c_str(), &s) == 0) {
      if (test & FILE_IS_REGULAR) {
        const bool regular(S_ISREG(s.st_mode));

        if (regular) {
          result |= FILE_IS_REGULAR;
        }
        test &= ~FILE_IS_REGULAR;
      }

      if (test & FILE_IS_DIRECTORY) {
        const bool dir(S_ISDIR(s.st_mode) != 0);

        if (dir) {
          result |= FILE_IS_DIRECTORY;
        }
        test &= ~FILE_IS_DIRECTORY;
      }

      if (test & FILE_IS_EXECUTABLE) {
        // The extra test for root when access (file, X_OK) success.
        if ((s.st_mode & S_IXOTH) ||
          (s.st_mode & S_IXUSR) ||
          (s.st_mode & S_IXGRP))
        {
          result |= FILE_IS_EXECUTABLE;
        }
        test &= ~FILE_IS_EXECUTABLE;
      }
    }
  }

  return (result);
} // OS::testFile

bool
OS::mkdirp(const std::string& path)
{
  if (path.empty()) { return (false); }

  if (testFile(path, OS::FILE_IS_DIRECTORY) != 0) {
    LogDebug("mkdirp [" << path << "] already exists.\n");
    return (true);
  }

  LogDebug("mkdirp [" << path << "]\n");
  const char * in = &*path.begin();
  std::string tmp;
  tmp += *in++;

  for (;;) {
    if ((*in == '\0') || (*in == '/')) {
      if (my_mkdir(tmp.c_str())) { return (false); }
    }

    tmp += *in;

    if (!*in) { break; }
    ++in;
  }

  LogDebug("mkdir [" << path << "] returns true\n");
  return (true);
}

namespace {
const char * const fallback_tmp_dir = "/tmp";
}

std::string
OS::getUniqueTemporaryFile(const std::string& base_in)
{
  // We actually touch file then return name, this makes sure it already
  // exists before being used by caller.  Avoids conflicts with other
  // instances of us
  std::string temp("/tmp/" + base_in + "XXXXXX");
  char fname[PATH_MAX];
  strcpy(fname, temp.c_str());
  int fd = mkstemp(fname);
  LogSevere("FILE IS " << std::string(fname) << "\n");
  close(fd);
  exit(1);
  return std::string(fname);
}
