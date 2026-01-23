#include <rDirWalker.h>
#include <rError.h>
#include <rTime.h>
#include <rStrings.h>

using namespace rapio;

thread_local DirWalker * DirWalker::myCurrentWalker;
thread_local std::string DirWalker::myCurrentRoot;

std::string
DirWalker::getCurrentRoot()
{
  return myCurrentRoot;
}

void
DirWalker::printPath(const std::string& prefix, const std::string& path, const struct stat * info)
{
  std::string x = getCurrentRoot(); // Roots can be a bit long for printing...
  std::string f = path;

  Strings::removePrefix(f, x);
  fLogInfo("{}...{} {} {}", prefix, f, Time(info->st_mtime, 0), Strings::formatBytes(info->st_size));
}

int
DirWalker::nftwCallback(const char * filePath, const struct stat * fileInfo, int typeFlag, struct FTW * pathInfo)
{
  // Pass onto the class so we can override, etc.
  return (myCurrentWalker->process(filePath, fileInfo, typeFlag, pathInfo));
}

bool
DirWalker::traverse(const std::string& path)
{
  int flags = FTW_PHYS; // Follow symbolic links (FIXME: make configurable)

  // Extra info for callback
  myCurrentWalker = this;
  myCurrentRoot   = path;

  fLogInfo("Traversing {}", path);
  if (nftw(path.c_str(),
    // Compiler can link a static c++ member function to c ok,
    // Have to be careful with extern "C", etc.
    &DirWalker::nftwCallback,
    10, flags) == -1)
  {
    fLogSevere("Failed to traverse directory.");
    return false;
  }
  return true;
}

int
DirWalker::process(const char * filePath, const struct stat * fileInfo, int type, struct FTW * pathInfo)
{
  // fileInfo->st_mtime;
  // fileInfo->st_size;
  // printf("0%3o\t%s/ (Directory)\n", status->st_mode&0777, name);
  // fLogInfo("(virtual) PATH IS {} {} {}", filePath, fileInfo->st_mtime, fileInfo->st_size);
  std::string aFilePath(filePath); // go back to c++ for interface consistency

  /** pathInfo is a FTW which contains the following.
   *  struct FTW {
   *         int base;
   *         int level;
   *     };
   * base is the offset of the filename (i.e., basename component) in the pathname given  in  fpath.
   * level  is  the depth of fpath in the directory tree, relative to the root of the tree (dirpath,
   * which has depth 0).
   */

  // Convert to hide the internals
  //  file depth is always the directory + 1 (it counts as another depth)
  myDepth      = pathInfo->level;
  myFileOffset = pathInfo->base;

  DirWalker::Action a = Action::CONTINUE;

  switch (type) {
      case FTW_F:
        // Handle a regular file
        myFileCounter++;
        a = processRegularFile(aFilePath, fileInfo);
        break;
      case FTW_D:
        // Handle a directory
        a = processDirectory(aFilePath, fileInfo);
        break;
      case FTW_DNR:
        // Handle a directory that could not be read
        break;
      case FTW_DP:
        // Handle a directory being visited after all subdirs have been visited
        break;
      case FTW_NS:
        // Handle a file/directory where 'stat' or 'lstat' failed
        break;
      case FTW_SL:
        // Handle a symbolic link
        break;
      case FTW_SLN:
        // Handle a symbolic link pointing to a nonexistent file/directory
        break;
      default:
        // Handle unrecognized flag
        break;
  }

  return static_cast<int>(a);
} // DirWalker::process

DirWalker::Action
DirWalker::processRegularFile(const std::string& filePath, const struct stat * info)
{
  printPath("File: ", filePath, info);
  return Action::CONTINUE;
}

DirWalker::Action
DirWalker::processDirectory(const std::string& dirPath, const struct stat * info)
{
  printPath("Dir: ", dirPath, info);
  return Action::CONTINUE;
}
