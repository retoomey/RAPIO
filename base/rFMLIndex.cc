#include "rFMLIndex.h"

#include "rIOIndex.h"
#include "rXMLIndex.h"
#include "rError.h"
#include "rIOURL.h"
#include "rOS.h"
#include "rRecordQueue.h"

#include <algorithm>
#include <errno.h>

#include <dirent.h>

using namespace rapio;

/** Default constant for a FAM polling index */
const std::string FMLIndex::FMLINDEX_FAM = "ifam";

/** Default constant for a polling FML index */
const std::string FMLIndex::FMLINDEX_POLL = "ipoll";

FMLIndex::~FMLIndex()
{
  /* Already destroyed.  Need a clean up exit caller
   * std::shared_ptr<WatcherType> watcher = IOWatcher::getIOWatcher("fam");
   * if (watcher != nullptr){
   *  watcher->detach(this);
   * }
   */
}

FMLIndex::FMLIndex(
  const std::string                                 & protocol,
  const URL                                         & url,
  const std::vector<std::shared_ptr<IndexListener> >& listeners,
  const TimeDuration                                & maximumHistory) :
  IndexType(listeners, maximumHistory),
  myProtocol(protocol),
  myIndexPath(IOIndex::getIndexPath(url))
{
  myURL = url; // duplicated?
}

bool
FMLIndex::wantFile(const std::string& path)
{
  // Ignore the 'system' stuff '.' and '..'
  if ((path.size() < 2) || (path[0] == '.')) {
    return false;
  }
  bool wanted = false;

  // ".fml" only please.
  // This is very picky.  Could make it case insensitive
  const size_t s = path.size();

  if ((s > 3) && (path[s - 4] == '.') &&
    (path[s - 3] == 'f') &&
    (path[s - 2] == 'm') &&
    (path[s - 1] == 'l')
  )
  {
    wanted = true;
  }
  return (wanted);
}

bool
FMLIndex::initialRead(bool realtime)
{
  if (!myURL.isLocal()) {
    LogSevere("Can't do an FML index off a remote URL at moment\n");
    return false;
  }
  const std::string loc = myURL.path;

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
      if (wantFile(dp->d_name)) {
        const std::string full = loc + "/" + dp->d_name;

        if (!OS::isDirectory(full)) {
          // Add record to queue
          Record r;
          if (fileToRecord(full, r)) {
            Record::theRecordQueue->addRecord(r);
          }
        }
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
    if (myProtocol == FMLIndex::FMLINDEX_FAM) {
      poll = "fam";
    } else if (myProtocol == FMLINDEX_POLL) {
      poll = "dir"; // FIXME: Still magic string here
    } else {
      LogSevere("FML index doesn't recognize polling protocol '" << myProtocol << "', trying inotify\n");
    }
    std::shared_ptr<WatcherType> watcher = IOWatcher::getIOWatcher(poll);
    bool ok = watcher->attach(loc, this);
    // watcher->attach("/home/dyolf/FAM2/", this);
    // watcher->attach("/home/dyolf/FAM3/", this);
    if (!ok) { return false; }
  }

  return true;
} // FMLIndex::initialRead

bool
FMLIndex::fileToRecord(const std::string& filename, Record& rec)
{
  // For now, tell .fml to be parsed as xml builder
  auto doc = IODataType::read<XMLData>(filename, "xml");

  if (doc == nullptr) {
    LogSevere("Unable to parse " << filename << "\n");
    return false;
  }
  auto tree = doc->getTree();

  /*
   * auto meta = doc->getChildOptional("meta");
   * FIXME: Use meta data?
   */
  try{
    auto item = tree->getChild("item");
    return (rec.readXML(item.node, myIndexPath, getIndexLabel()));
  }catch (std::exception& e) {
    LogSevere("Missing item tag in FML record\n");
  }
  return false;
}

void
FMLIndex::handleNewFile(const std::string& filename)
{
  if (wantFile(filename)) {
    Record rec;
    if (fileToRecord(filename, rec)) {
      // Add to the record queue.  Never process here directly.  The queue
      // will call our addRecord when it's time to do the work
      Record::theRecordQueue->addRecord(rec);
    }
  }
}

void
FMLIndex::handleUnmount(const std::string& dirname)
{
  LogSevere("Our FAM watch " << dirname << " was unmounted!\n");
  LogSevere("Restarting algorithm/program.\n");
  exit(1);
}

void
FMLIndex::introduceSelf()
{
  // input FAM ingestor
  std::shared_ptr<IndexType> newOne = std::make_shared<FMLIndex>();
  // Handle a fml index getting stuff from fam
  IOIndex::introduce(FMLINDEX_FAM, newOne);
  // Handle a fml index getting stuff from a directory poll
  IOIndex::introduce(FMLINDEX_POLL, newOne);
}

std::shared_ptr<IndexType>
FMLIndex::createIndexType(
  const std::string                            & protocol,
  const URL                                    & location,
  std::vector<std::shared_ptr<IndexListener> > listeners,
  const TimeDuration                           & maximumHistory)
{
  std::shared_ptr<FMLIndex> result = std::make_shared<FMLIndex>(
    protocol,
    location,
    listeners,
    maximumHistory);
  return (result); // Factory handles isValid now...
}
