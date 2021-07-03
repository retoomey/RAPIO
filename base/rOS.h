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
   * Returns the process name.
   */
  static std::string
  getProcessName();

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

  /**
   * Run a OS process
   */
  static std::vector<std::string>
  runProcess(const std::string& command);

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
};
}
