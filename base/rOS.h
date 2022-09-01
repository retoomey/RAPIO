#pragma once

#include <rUtility.h>
#include <rDataGrid.h>

#include <string>
#include <vector>

#include <dlfcn.h>

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
  getProcessSize(double& vm, double& rss);

  /**
   * Rename a file path, even between different file systems
   */
  static bool
  moveFile(const std::string& from, const std::string& to);

  /**
   * Delete a file
   */
  static bool
  deleteFile(const std::string& file);

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

  /** Check endianness  */
  static bool
  isBigEndian()
  {
    // Humm even better, set a global variable on start up I think...
    static bool firstTime = true;
    static bool big       = false;

    if (!firstTime) {
      union {
        uint32_t i;
        char     c[4];
      } bint = { 0x01020304 };
      big       = (bint.c[0] == 1);
      firstTime = false;
    }
    return big;
  }
};
}
