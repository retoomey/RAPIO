#pragma once

#include <rUtility.h>

#include <string>
#include <vector>

namespace rapio {
/**
 * A utility for common system calls.
 * It might be better to migrate utilities to sections depending
 * on what they do. Currently these are file and network utilities.
 *
 */
class OS : public Utility {
public:

  /**
   * Get the hostname of machine
   */
  static const std::string&
  getHostName();

  /**
   * Returns the process name.
   */
  static std::string
  getProcessName();

  /**
   * Test flags that can be passed to testFile().
   */
  static const unsigned int FILE_IS_REGULAR;
  static const unsigned int FILE_IS_SYMLINK;
  static const unsigned int FILE_IS_DIRECTORY;
  static const unsigned int FILE_IS_EXECUTABLE;
  static const unsigned int FILE_EXIST;

  /**
   * Test a file for the properties specified in test_flags.
   *
   * test_flags can be a bitwise-or'ed set of
   * FILE_IS_REGULAR, FILE_IS_SYMLINK, FILE_IS_DIRECTORY,
   * FILE_IS_EXECUTABLE, and FILE_EXIST.
   *
   * @return the subset of test_flags which are true.
   */
  static unsigned int
  testFile(const std::string& filename,
    unsigned int            test_flags);

  /**
   * Returns the current working directory.
   */
  static std::string
  getCurrentDirectory();

  /**
   * Create all the directories necessary for the given path.
   * Doesn't exist on all Unixes.
   * One workaround, system("mkdir -p dirname"), doesn't work
   * on Windows.
   *
   * @return false if the path does not already exist and
   *  could not be created new.
   */
  static bool
  mkdirp(const std::string& path);

  /**
   * Return a unique temporary name.
   * No one else will have this name while this executable is running.
   * If you want to use it as a directory, use getUniqueTemporaryDir().
   *
   * the 'base' string will be the lead part of the basename,
   * and some qualifier will be added to it to ensure
   * uniqueness; ie /tmp/base.some-unique-qualifier
   */
  static std::string
  getUniqueTemporaryFile(const std::string& base = "wdssii");
};
}
