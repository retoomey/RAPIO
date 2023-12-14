#pragma once

#include <rUtility.h>
#include <rDataGrid.h>

#include <string>
#include <vector>

#include <dlfcn.h>

// Endian check can be compile time (saves a little speed)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
# define ON_BIG_ENDIAN(...)
# define IS_BIG_ENDIAN 0
#else
# define ON_BIG_ENDIAN(func) func
# define IS_BIG_ENDIAN 1
#endif

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
# define ON_LITTLE_ENDIAN(...)
#else
# define ON_LITTLE_ENDIAN(func) func
#endif

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
   * Returns the process name. Default is full path,
   * but passing flag will remove folders
   */
  static std::string
  getProcessName(const bool shortname = false);

  /**
   * Gets root path of process.
   */
  static std::string
  getProcessPath();

  /**
   * Returns the process id.
   */
  static int
  getProcessID();

  /** Test if path is a directory */
  static bool
  isDirectory(const std::string& path);

  /** Ensure full directory path exists, try to create as well */
  static bool
  ensureDirectory(const std::string& path);

  /** Test if path is a regular file */
  static bool
  isRegularFile(const std::string& path);

  /** Canonicalize a path (remove ../, etc. ) */
  static std::string
  canonical(const std::string& path);

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

  /** Return root file extension using OS ability, for example:
   * xml.gz returns 'xml', and also returns the non-ending part */
  static std::string
  getRootFileExtension(const std::string& path, std::string& prefix);

  /** Return root file extension using OS ability, for example:
   * xml.gz returns 'xml' */
  static std::string
  getRootFileExtension(const std::string& path);

  /** Validate executable path */
  static std::string
  validateExe(const std::string& path);

  /** Find a valid executable path from a list of relative and absolute paths,
   * returning the first one.  For example: python, /usr/bin/python2 */
  static std::string
  findValidExe(const std::vector<std::string>& list);

  /**
   * Run a OS process, return true is successful and lines of output
   * Returns exit code of process or -1 on general failure
   */
  static int
  runProcess(const std::string& command, std::vector<std::string>& output);

  /**
   * Run a python data process (or others if they know the API), passing JSON and data
   * as shared memory
   * FIXME: Make it data type (more general)
   */
  static std::vector<std::string>
  runDataProcess(const std::string& command, std::shared_ptr<DataGrid> datagrid);

  /** Load a dynamic module */
  template <class T> static std::shared_ptr<T>
  loadDynamic(const std::string& module, const std::string& function)
  {
    void * library = dlopen(module.c_str(), RTLD_LAZY);

    if (library == NULL) {
      LogSevere("Cannot load requested module: " << module << ":" << dlerror() << "\n");
    } else {
      // dlerror(); // why this here?
      LogInfo("Loaded requested module: " << module << "\n");
      typedef T * create_h ();
      create_h * create_handler = (create_h *)  dlsym(library, function.c_str());
      const char * dlsym_error  = dlerror();
      if (dlsym_error) {
        LogSevere("Missing " << function << " routine in module " << module << "\n");
      } else {
        T * working = create_handler();
        if (working != nullptr) {
          std::shared_ptr<T> newOne(working);
          return newOne;
        }
      }
    }
    return nullptr;
  }

  /**
   * The current virtual memory and resident set size in KB for this program.
   */
  static void
  getProcessSizeKB(double& vm, double& rss);

  /**
   * Copy a file path, atomic even between different file systems
   */
  static bool
  copyFile(const std::string& from, const std::string& to);

  /**
   * Rename a file path, atomic even between different file systems
   */
  static bool
  moveFile(const std::string& from, const std::string& to);

  /**
   * Delete a file
   */
  static bool
  deleteFile(const std::string& file);

  /** Get the file size in bytes for a file, or 0 if unreadable */
  static size_t
  getFileSize(const std::string& file);

  /** Get the file modification time */
  static bool
  getFileModificationTime(const std::string& file, Time& time);

  /** Swap bytes for a given type for endian change */
  template <class T>
  static void
  byteswap(T &data)
  {
    size_t N = sizeof( T );

    if (N == 1) { return; }
    size_t mid = floor(N / 2); // Even 4 --> 2 Odd 5 --> 2 (middle is already good)

    char * char_data = reinterpret_cast<char *>( &data );

    for (size_t i = 0; i < mid; i++) { // swap in place
      const char temp = char_data[i];
      char_data[i]         = char_data[N - i - 1];
      char_data[N - i - 1] = temp;
    }
  }

  /**
   * Find and return an environment variable.
   * This string is empty if there is no such environment variable.
   * Wrapper to the getenv method.
   */
  static std::string
  getEnvVar(const std::string& envVarName);

  /** Try to set this environment variable */
  static void
  setEnvVar(const std::string& envVarName, const std::string& value);

  /** Determine if we're running in a container or not */
  static bool
  isContainer();

  /** Run a command on a file supporting file macros */
  static bool
  runCommandOnFile(const std::string& postCommandIn, const std::string& finalFile, bool captureOut = true);
};
}
