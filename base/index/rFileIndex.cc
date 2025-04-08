#include "rFileIndex.h"

#include "rIOIndex.h"
#include "rError.h"
#include "rIOURL.h"
#include "rOS.h"
#include "rRecordQueue.h"
#include "rDirWatcher.h"
#include "rStrings.h"
#include "rIODataType.h"

#include <algorithm>
#include <errno.h>

#include <dirent.h>

using namespace rapio;

/** Default constant for a FAM polling index */
const std::string FileIndex::FileINDEX_FAM = "fam";

/** Default constant for a polling File index */
const std::string FileIndex::FileINDEX_POLL = "poll";

/** Default constant for processing a single file */
const std::string FileIndex::FileINDEX_ONE = "file";

FileIndex::~FileIndex()
{ }

FileIndex::FileIndex(
  const std::string  & protocol,
  const URL          & aURL,
  const TimeDuration & maximumHistory) :
  IndexType(maximumHistory),
  myProtocol(protocol),
  myURL(aURL),
  myIndexPath(IOIndex::getIndexPath(aURL))
{ }

std::string
FileIndex::getHelpString(const std::string& fkey)
{
  if (fkey == FileINDEX_FAM) {
    return "Use inotify to watch a directory for general data files (such as for an ingestor).\n  Example: "
           + FileINDEX_FAM + "=/mydata";
  } else if (fkey == FileINDEX_POLL) {
    return "Use polling to watch a directory for general data files (such as for an ingestor).\n  Example: "
           + FileINDEX_POLL + "=/mydata";
  } else {
    return "Process a single file, providing builder or guessing from the file suffix.\n  Example: " + FileINDEX_ONE
           + "=/mydata/netcdffile.netcdf\n  Example: " + FileINDEX_ONE + "=hmrg:REFL.20220202.222000.gz";
  }
}

bool
FileIndex::initialRead(bool realtime, bool archive)
{
  if (!myURL.isLocal()) {
    LogSevere("Can't do this index off a remote URL at moment.n");
    return false;
  }
  const std::string loc = myURL.getPath();

  // Choose FAM or POLL watcher depending on protocol
  std::string poll = FAMWatcher::FAM_WATCH;

  if (myProtocol == FileIndex::FileINDEX_FAM) {
    poll = FAMWatcher::FAM_WATCH;
  } else if (myProtocol == FileINDEX_POLL) {
    poll = DirWatcher::DIR_WATCH;
  } else if (myProtocol == FileINDEX_ONE) {
    // Ok no watcher required or anything, we're just trying to read a single file
    // and we're done.  Remember though other indexes might not be done, so we just
    // handle one directly
    handleFile(loc);
    return true;
  } else {
    LogSevere("File index doesn't recognize polling protocol '" << myProtocol << "', trying inotify\n");
  }

  std::shared_ptr<WatcherType> watcher = IOWatcher::getIOWatcher(poll);
  bool ok = watcher->attach(loc, realtime, archive, this);

  // watcher->attach("/home/dyolf/FAM2/", this);
  // watcher->attach("/home/dyolf/FAM3/", this);
  if (!ok) { return false; }

  return true;
} // FileIndex::initialRead

bool
FileIndex::wantFile(const std::string& path)
{
  return true;
}

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
  } else if (m == "unmountr") {
    handleUnmount(d, true);
  }
}

void
FileIndex::handleFile(const std::string& filename)
{
  if (wantFile(filename)) {
    // Create record from filename as best we can...
    // For first pass just want the polling to work

    // Use stat to set time of record?
    Time time = Time::CurrentTime();

    std::vector<std::string> params;

    // Ok we're gonna allow explicit builder specification so
    // 1. file=netcdf:/pathtonetcdf or
    // 2. file=stuff.netcdf(.gz) and we pull the suffix off
    std::string aBuilder;
    std::string aFileName;

    // We're getting our path from -i at moment...
    IODataType::iPathParse(filename, aFileName, aBuilder);

    // Create params for the record
    params.push_back(aBuilder);
    params.push_back(aFileName);

    // Add record for the file
    Record rec(params, aBuilder, time, "default", "file");
    Record::theRecordQueue->addRecord(rec);
  }
} // FileIndex::handleFile

void
FileIndex::handleNewDirectory(const std::string& dirname)
{
  LogInfo("New directory was added: " << dirname << "\n");
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
  IOIndex::introduce(FileINDEX_ONE, newOne);
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
