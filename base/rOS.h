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

// Macro for conditional byte-swapping
#define ON_ENDIAN_DIFF(dataIsBigEndian, func) \
  if (dataIsBigEndian != IS_BIG_ENDIAN) { func; }

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

  /** Canonicalize a path (remove ../, etc. ) This requires path to exist on disk. */
  static std::string
  canonical(const std::string& path);

  /** Validate a file name path characters (remove double // or space, etc.) */
  static std::string
  validatePathCharacters(const std::string& path);

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

  /**
   * Run a capturable function in our code.  For example, the wgrib2 c function api
   * is built into the iowgrib module.  This captures stdout from it to a vector
   * of strings.  It's a rare use case but 'might' happen again so we'll
   * generalize it.
   *
   * @tparam Func The type of the callable (e.g., function pointer, lambda, std::function).
   * @tparam Args The types of the arguments passed to the callable.
   * @param func The function/callable to execute.
   * @param args The arguments to pass to the callable.
   * @return A vector of strings containing the captured stdout, line by line.
   */

  template <typename Func, typename ... Args>
  static std::vector<std::string>
  runFunction(bool capture, Func&& func, Args&&... args)
  {
    std::vector<std::string> voutput;

    // Allow a direct call/return for convenience
    if (!capture) {
      std::forward<Func>(func)(std::forward<Args>(args)...);
      return voutput;
    }

    // Ahh. Since we're calling a function and not spawning an external
    // binary:
    // 1. We need to make sure other threads don't write to our wgrib2
    // stream while we're capturing output, or it gets corrupted.
    // Even if we switch to asynchronous logging (eventually) we'll need
    // to delay other threads from printing to console.
    const std::lock_guard<std::recursive_mutex> log_lock(Log::logLock);

    // 2. If any callback class (called from the wgrib2 call) in the same thread
    // tries to log we don't want that so pause logging
    // Note: If any callback does std::cout directly or anything this will
    // break again.  At least we've fixed standard logging here.
    Log::pauseLogging();

    // 3. Flush all old stdout messages/std::cout in buffer.  Now hopefully
    // nothing else writes from our code while we capture.  Note, if a
    // callback class here writes to std::cout, it will go to stdout on flush
    // and it will probably corrupt the catalog.
    std::cout.flush();
    fflush(stdout);

    // 4. Setup Pipe and Redirection
    int pipefd[2];

    if (pipe(pipefd) == -1) {
      Log::restartLogging();
      return voutput;
    }

    // Save original stdout
    int stdout_copy = dup(STDOUT_FILENO);

    // Redirect stdout to pipe's write end
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[1]); // Close the redundant write end FD

    // 4. Run reader thread.  Important in case buffer fills
    // we don't want reader/writer deadlock (writer waiting on reader)
    auto reader_func = [](int read_fd, std::vector<std::string>& output) {
        // ... (The exact same pipe-reading loop as before) ...
        std::string partial_line;
        char buffer[4096];
        ssize_t count;

        while ((count = ::read(read_fd, buffer, sizeof(buffer))) > 0) {
          size_t start = 0;
          for (ssize_t i = 0; i < count; ++i) {
            if (buffer[i] == '\n') {
              partial_line.append(&buffer[start], i - start);
              output.push_back(std::move(partial_line));
              partial_line.clear();
              start = i + 1;
            }
          }
          if (start < count) {
            partial_line.append(&buffer[start], count - start);
          }
        }
        if (!partial_line.empty()) {
          output.push_back(std::move(partial_line));
        }
        close(read_fd);
      };

    // Create and start the reader thread
    std::thread reader_thread(reader_func, pipefd[0], std::ref(voutput));

    // 4. Run the Callable Object (The 'Writer')
    // Uses std::forward to correctly handle lvalues/rvalues for arguments
    std::forward<Func>(func)(std::forward<Args>(args)...);

    // 5. Restore and Cleanup
    fflush(stdout);
    dup2(stdout_copy, STDOUT_FILENO);
    close(stdout_copy);

    if (reader_thread.joinable()) {
      reader_thread.join();
    }

    Log::restartLogging();
    return voutput;
  } // runFunction

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
  moveFile(const std::string& from, const std::string& to, bool quiet = false);

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

  /** Determine if we're running in WSL window's subsystem for linux  or not */
  static bool
  isWSL();

  /** Run a command on a file supporting file macros */
  static bool
  runCommandOnFile(const std::string& postCommandIn, const std::string& finalFile, bool captureOut = true);
};
}
