#include "rFileIndex.h"

#include "rIOIndex.h"
#include "rError.h"
#include "rIOURL.h"
#include "rOS.h"
#include "rRecordQueue.h"

#include <algorithm>
#include <errno.h>

#include <dirent.h>

using namespace rapio;

/** Default constant for a FAM polling index */
const std::string FileIndex::FileINDEX_FAM = "fam";

/** Default constant for a polling File index */
const std::string FileIndex::FileINDEX_POLL = "poll";

FileIndex::~FileIndex()
{
  /* Already destroyed.  Need a clean up exit caller
   * std::shared_ptr<WatcherType> watcher = IOWatcher::getIOWatcher("fam");
   * if (watcher != nullptr){
   *  watcher->detach(this);
   * }
   */
}

FileIndex::FileIndex(
  const std::string  & protocol,
  const URL          & aURL,
  const TimeDuration & maximumHistory) :
  IndexType(maximumHistory),
  myProtocol(protocol),
  myURL(aURL),
  myIndexPath(IOIndex::getIndexPath(aURL))
{ }

bool
FileIndex::initialRead(bool realtime)
{
  if (!myURL.isLocal()) {
    LogSevere("Can't do a file index off a remote URL at moment.n");
    return false;
  }
  const std::string loc = myURL.getPath();

  // ---------------------------------------------------------
  // Archive
  //
  // Grab the list of files currently in the directory

  if (!realtime) {
    DIR * dirp = opendir(loc.c_str());
    if (dirp == 0) {
      LogSevere("Unable to read location " << loc << "\n");
      return (false);
    }
    struct dirent * dp;
    while ((dp = readdir(dirp)) != 0) {
      // Humm adding string here might be wasteful...
      const std::string full = loc + "/" + dp->d_name;
      if (!OS::isDirectory(full)) {
        handleFile(full);
      }
    }
    closedir(dirp);
  }

  // ---------------------------------------------------------
  // Realtime
  //
  if (realtime) {
    // Connect to FAM, we'll get notified of new files
    std::string poll = "fam";
    if (myProtocol == FileIndex::FileINDEX_FAM) {
      poll = "fam";
    } else if (myProtocol == FileINDEX_POLL) {
      poll = "dir"; // FIXME: Still magic string here
    } else {
      LogSevere("File index doesn't recognize polling protocol '" << myProtocol << "', trying inotify\n");
    }
    std::shared_ptr<WatcherType> watcher = IOWatcher::getIOWatcher(poll);
    bool ok = watcher->attach(loc, this);
    // watcher->attach("/home/dyolf/FAM2/", this);
    // watcher->attach("/home/dyolf/FAM3/", this);
    if (!ok) { return false; }
  }

  return true;
} // FileIndex::initialRead

void
FileIndex::handleNewEvent(WatchEvent * event)
{
  const auto& m = event->myMessage;
  const auto& d = event->myData;

  if (m == "newfile") {
    handleFile(d);
  } else if (m == "newdir") {
    handleNewDirectory(d);
  } else if (m == "unmount") {
    handleUnmount(d, false);
    exit(1);
  } else if (m == "unmountr") {
    handleUnmount(d, true);
  }
}

void
FileIndex::handleFile(const std::string& filename)
{
  LogSevere("HANDLE:" << filename << "\n");

  /*
   * if (wantFile(filename)) {
   *  Record rec;
   *  if (fileToRecord(filename, rec)) {
   *    // Add to the record queue.  Never process here directly.  The queue
   *    // will call our addRecord when it's time to do the work
   *    Record::theRecordQueue->addRecord(rec);
   *  }
   * }
   */
}

void
FileIndex::handleNewDirectory(const std::string& dirname)
{
  LogInfo("New directory was added: " << dirname << "\n");
  LogSevere("HANDLE DIR:" << dirname << "\n");
}

void
FileIndex::handleUnmount(const std::string& dirname, bool willReconnect)
{
  LogSevere("Directory " << dirname << " was unmounted or removed.\n");
  if (!willReconnect) {
    LogSevere("Stopping algorithm due to lost input directory.\n");
    exit(1);
  }
}

void
FileIndex::introduceSelf()
{
  std::shared_ptr<IndexType> newOne = std::make_shared<FileIndex>();
  // Handle from FAM and POLL
  IOIndex::introduce(FileINDEX_FAM, newOne);
  IOIndex::introduce(FileINDEX_POLL, newOne);
}

std::shared_ptr<IndexType>
FileIndex::createIndexType(
  const std::string  & protocol,
  const std::string  & indexparams,
  const TimeDuration & maximumHistory)
{
  std::shared_ptr<FileIndex> result = std::make_shared<FileIndex>(
    protocol,
    URL(indexparams),
    maximumHistory);
  return (result); // Factory handles isValid now...
}
