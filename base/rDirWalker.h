#pragma once

#include <rUtility.h>

// Use c calls in linux current (could swap out with c++17 filesystem at some point)
extern "C" {
#include <ftw.h>
#include <sys/stat.h>
}

// Alpine or libc vs glib linux versions
#ifdef __USE_GNU
#else
enum {
  FTW_CONTINUE      = 0,
  FTW_STOP          = 1,
  FTW_SKIP_SIBLINGS = 2,
  FTW_SKIP_SUBTREE  = 8
};
#endif

namespace rapio {
/** A class that allows walking of files/directories.
 * In operations were stuck to c++11 at moment so we can't use c++17 filesystem
 * so we'll wrap and later we have the option of replacing with c++17 calls.
 * The default class just dumps the files and some info on each.
 *
 * Override processRegularFile, processDirectory to handle things. Each of those
 * can return a future action for the directory walk.  We currently pass a c
 * style stat with these current fields:
 *
 *   - st_mode: File type and mode (permissions).
 *   - st_ino: Inode number.
 *   - st_dev: Device ID.
 *   - st_nlink: Number of hard links.
 *   - st_uid: User ID of the file's owner.
 *   - st_gid: Group ID of the file's group.
 *   - st_size: Size of the file in bytes.
 *   - st_atime: Last access timestamp (atime).
 *   - st_mtime: Last modification timestamp (mtime).
 *   - st_ctime: Last status change timestamp (ctime).
 *   - st_blksize: Optimal I/O block size for the file.
 *   - st_blocks: Total number of 512-byte blocks allocated to the file.
 *
 * @author Robert Toomey
 */
class DirWalker : public Utility {
public:

  /** @enum DirWalkerAction
   *  Processing directives (return after processing a file, directory, etc.).
   *  This allows us to hide the underlying method of action.
   */
  enum class Action {
    /** Continue processing. Keep traversing the directory tree. */
    CONTINUE      = FTW_CONTINUE,

    /** Skip processing siblings. Skip siblings of the current item being processed. */
    SKIP_SIBLINGS = FTW_SKIP_SIBLINGS,

    /** Skip processing subtree. Skip any subdirectories of the current directory. */
    SKIP_SUBTREE  = FTW_SKIP_SUBTREE,

    /** Stop processing. Stop traversing the directory tree. */
    STOP          = FTW_STOP
  };

  /** Create default walker with flags */
  DirWalker() : myDepth(0), myFileOffset(0), myFileCounter(0){ }

  /** Do a directory walk */
  bool
  traverse(const std::string& path);

  /** Process a regular file */
  virtual Action
  processRegularFile(const std::string& filePath, const struct stat * info);

  /** Process a directory */
  virtual Action
  processDirectory(const std::string& dirPath, const struct stat * info);

  /** Get the current root directory passed to traverse (if currently walking) */
  static std::string
  getCurrentRoot();

  /** Convenience path dump with some info (callable from process methods) */
  static void
  printPath(const std::string& prefix, const std::string& path, const struct stat * info);

  /** Get current directory depth of traversal */
  size_t getDepth(){ return myDepth; }

  /** Get current local file offset in the full file name */
  size_t getFileOffset(){ return myFileOffset; }

  /** Get current file number processed */
  size_t getFileCounter(){ return myFileCounter; }

private:

  /** Top level callback from hidden nftw details. */
  int
  process(const char * filePath, const struct stat * fileInfo, int typeFlag, struct FTW * pathInfo);

  /** Current directory walker */
  static thread_local DirWalker * myCurrentWalker;

  /** Current directory traversing */
  static thread_local std::string myCurrentRoot;

  /** Directly static callback for nftw, not to be changed */
  static int
  nftwCallback(const char * filePath, const struct stat * fileInfo, int typeFlag, struct FTW * pathInfo);

  /** Directory depth counter */
  size_t myDepth;

  /** File offset in current full file name path */
  size_t myFileOffset;

  /** File counter */
  size_t myFileCounter;
};
}
