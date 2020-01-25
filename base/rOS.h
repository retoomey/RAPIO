#pragma once

#include <rUtility.h>

#include <string>
#include <vector>

namespace rapio {
/**
 * A utility for common system calls.
 * @author Robert Toomey
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

  /** Test if path is a directory */
  static bool
  isDirectory(const std::string& path);

  /**
   * Returns the current working directory.
   */
  static std::string
  getCurrentDirectory();

  /**
   * Create all the directories necessary for the given path.
   *
   * @return false if the path does not already exist and
   *  could not be created new.
   */
  static bool
  mkdirp(const std::string& path);

  /**
   * Return a unique temporary name.
   *
   * Given the 'base' string, some qualifier will be added
   * to it to ensure uniqueness; ie /tmp/base.some-unique-qualifier
   */
  static std::string
  getUniqueTemporaryFile(const std::string& base = "wdssii");

  /**
   * Run a OS process
   */
  static std::vector<std::string>
  runProcess(const std::string& command);
};
}
