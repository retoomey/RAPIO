#include "rFAMWatcher.h"

#include "rError.h"
#include "rOS.h"
#include "rIOURL.h"

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/inotify.h>

#include <queue>
#include <algorithm>

using namespace rapio;

/** Global file instance for inotify */
int FAMWatcher::theFAMID = -1;

void
FAMWatcher::init()
{
  theFAMID = inotify_init();

  if (theFAMID < 0) {
    std::string errMessage(strerror(errno));
    LogSevere("inotify_init failed with error: " << errMessage << "\n");
  } else {
    int ret = fcntl(theFAMID, F_SETFL, O_NONBLOCK);

    if (ret == 0) {
      LogInfo("Successfully connected to inotify device\n");
    } else {
      LogSevere(
        "Unable to change inotify device to non-blocking mode. Programs may hang.\n");
    }
  }
}

namespace {
// Ignore the 'system' stuff... '.' and '..'
inline bool
shouldIgnore(const std::string& filename)
{
  return (filename.size() < 2 || filename[0] == '.');
}
}

bool
FAMWatcher::attach(const std::string &dirname,
  IOListener *                       l)
{
  /** Check if initial inotify creation worked */
  if (theFAMID < 0) {
    init();
    if (theFAMID < 0) { return (false); }
  }

  // create watch for this directory
  // based on mask to make sure we don't double respond to FAM events
  uint32_t mask = IN_CLOSE_WRITE | IN_MOVED_TO | IN_ONLYDIR |

    // We actually want IN_CREATE events, but only to add a new alg created
    // directory.
    // We don't handle renames (directory)/move FROM events (directory)..those break
    // that fam directory.
    // This is because most likely the algorithm is already hard pathed to that
    // directory and will break
    // anyway or recreate that dir.
    IN_CREATE;

  // linux 2.65.36+ (not redhat 5) Prevent mass delete spam events on
  // directory removal with stuff in it...
  #if IN_EXCL_UNLINK
  mask |= IN_EXCL_UNLINK;
  #endif // if IN_EXCL_UNLINK

  int watchID = inotify_add_watch(theFAMID, dirname.c_str(), mask);
  if (watchID < 0) {
    std::string errMessage(strerror(errno));
    LogSevere(
      "inotify_add_watch for " << dirname << " failed with error: " << errMessage
                               << "\n");
    return (false);
  }

  std::shared_ptr<FAMInfo> newWatch = std::make_shared<FAMInfo>(l, dirname, watchID);
  myWatches.push_back(newWatch);
  LogDebug(dirname << " being monitored ... \n");
  return true;
} // FAMWatcher::attach

void
FAMWatcher::addFAMEvent(IOListener * l,
  const std::string& directory, const std::string& shortfile, const uint32_t amask)
{
  // FIXME: configuration of wanted events maybe so they only get created if wanted

  if (amask & IN_IGNORED) { // Ignore deletions...
    // LogSevere("...deletion?\n");
    return;
  }
  // IOListener class should decide.  Maybe it doesn't care...
  if (amask & IN_UNMOUNT) { // End on an unmount and hope restart will get
                            // path again
    WatchEvent e(l, "unmount", directory);
    myEvents.push(e);
    return;
  }

  if (amask & IN_ISDIR) {
    // Are create and move the same?
    if ((amask & IN_CREATE) || (amask & IN_MOVE)) {
      WatchEvent e(l, "newdir", directory);
      myEvents.push(e);
    }
  } else {
    // Are create and move the same?
    if ((amask & IN_CLOSE_WRITE) || (amask & IN_MOVE)) {
      WatchEvent e(l, "newfile", directory + '/' + shortfile);
      myEvents.push(e);
    }
  }
}

void
FAMWatcher::getEvents()
{
  // Ignore global FAM polling if no watches left or exist
  if (myWatches.size() == 0) {
    return;
  }
  // initial guess: 5 new files per directory being watched?
  static std::vector<char> buf((5 * myWatches.size() + 10) * sizeof(inotify_event));
  size_t startSize;

  do {
    startSize = myEvents.size(); // Ok so zero normally?
    int len;

    // Start FAM poll ---------------------------------------------
    do {
      do {
        len = read(theFAMID, &(buf[0]), buf.size());
      } while (len < 0 && errno == EINTR);

      if (len < 0) {
        if (errno != EAGAIN) {
          perror("inotify_read: ");
          LogSevere("Reading inotify device failed err=" << errno << "\n");
        }
        // LogSevere("No fam events I think..\n");
        return;
      }

      if (len == 0) {
        buf.resize(buf.size() * 4);
        LogDebug("Resized buffer to " << buf.size() << " bytes.\n");
      }
    } while (len == 0);
    // End FAM poll -----------------------------------------------

    // Handle FAM events back...
    for (int i = 0; i < len;) {
      inotify_event * event = (inotify_event *) (&(buf[i]));

      const int wd = event->wd;

      // This is O(n) but ends up being faster than std::map in practice
      for (auto ww:myWatches) {
        // FIXME: I like reducing the code, but don't like this, maybe template
        auto * z = ww.get();
        auto * w = (FAMInfo *) (z);
        if (w->myWatchID == wd) {
          // Pre filter out events before even adding...
          bool want = true;
          if (event->mask & IN_IGNORED) { // Ignore deletions...
            want = false;
          } else if (event->mask & IN_UNMOUNT) {
            want = true;
          } else if (shouldIgnore(event->name)) {
            want = false;
          }
          if (want) {
            addFAMEvent(w->myListener, w->myDirectory, event->name, event->mask);
          }
          break;
        }
      }
      i = i + sizeof(inotify_event) + event->len;
    }
  } while (myEvents.empty() || myEvents.size() != startSize);
} // FAMWatcher::getEvents

void
FAMWatcher::introduceSelf()
{
  std::shared_ptr<WatcherType> io = std::make_shared<FAMWatcher>();
  IOWatcher::introduce("fam", io);
}

bool
FAMWatcher::FAMInfo::handleDetach(WatcherType * owner)
{
  int ret = inotify_rm_watch(FAMWatcher::getFAMID(), myWatchID);

  if (ret < 0) {
    std::string errMessage(strerror(errno));
    LogInfo("inotify_rm_watch on " << myDirectory << " Error: " << errMessage << "\n");
  }
  LogDebug(myDirectory << " no longer being monitored ... \n");
  return true;
}
