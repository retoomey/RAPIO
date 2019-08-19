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

/** Event queue for FAM */
namespace rapio {
static std::queue<FAMWatcher::FAMEvent> theEvents;
}

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

  myWatches.push_back(FAMInfo(l, dirname, watchID));
  LogDebug(dirname << " being monitored ... \n");
  return true;
} // FAMWatcher::attach

void
FAMWatcher::detach(IOListener * d)
{
  for (auto& w:myWatches) {
    // If listener at i matches us...
    if (w.myListener == d) {
      int ret = inotify_rm_watch(theFAMID, w.myWatchID);
      if (ret < 0) {
        std::string errMessage(strerror(errno));
        LogInfo("inotify_rm_watch on " << w.myDirectory << " Error: " << errMessage << "\n");
      }
    }
    LogDebug(w.myDirectory << " no longer being monitored ... \n");
    w.myListener = nullptr;
  }

  // Lamba erase all zeroed listeners
  myWatches.erase(
    std::remove_if(myWatches.begin(), myWatches.end(),
    [](const FAMInfo &w){
    return (w.myListener == nullptr);
  }),
    myWatches.end());
}

void
FAMWatcher::action()
{
  // FIXME: Not sure we need a fam queue since records themselves
  // are now queued.  However an ingestor might directly process
  // data on a handle call..so probably should keep it for now

  // Add to queue. Could be multiple fam files
  getEvents();

  // Do a _single_ action and don't hog the system.
  // Don't worry, event loop will give us as much time as is possible.
  if (!theEvents.empty()) {
    LogSevere("Pop a single one...\n");
    auto e = theEvents.front();
    theEvents.pop();

    // These should pass to the listener to decide maybe...
    // Handling this
    if (e.mask & IN_IGNORED) { // Ignore deletions...
      LogSevere("...deletion?\n");
      return;
    }
    // IOListener class should decide.  Maybe it doesn't care...
    if (e.mask & IN_UNMOUNT) { // End on an unmount and hope restart will get
                               // path again
      const std::string dir = e.getDirectory();
      e.myListener->handleUnmount(dir);
      return;
    }

    // Subclass should handle or not?
    if (shouldIgnore(e.name)) {
      LogSevere("...should ignore?\n");
      return; // Not a system . dir
    }

    if (e.mask & IN_ISDIR) {
      // Are create and move the same?
      if ((e.mask & IN_CREATE) || (e.mask & IN_MOVE)) {
        const std::string dir = e.getDirectory();
        e.myListener->handleNewDirectory(dir);
      }
    } else {
      // Are create and move the same?
      if ((e.mask & IN_CLOSE_WRITE) || (e.mask & IN_MOVE)) {
        const std::string full = e.getFullName();
        e.myListener->handleNewFile(full);
      }
    }
  }
} // FAMWatcher::action

void
FAMWatcher::getEvents()
{
  // Let watches do stuff too..however with FAM they will wait for the traditional
  // file events from us.
  for (auto&w:myWatches) {
    w.myListener->handlePoll();
  }

  size_t MAX_QUEUE_SIZE = 1000;
  bool WAIT_WHEN_FULL   = true;

  if (myWatches.size() == 0) {
    return;
  }

  // If queue is full do we wait or drop oldest...wait when full
  // we don't poll for events.
  if (theEvents.size() == MAX_QUEUE_SIZE) {
    if (WAIT_WHEN_FULL) {
      LogSevere("FAM queue is full..waiting.\n");
      return;
    }
  }

  // initial guess: 5 new files per directory being watched?
  static std::vector<char> buf((5 * myWatches.size() + 10) * sizeof(inotify_event));
  size_t startSize;

  do {
    startSize = theEvents.size(); // Ok so zero normally?
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
      for (auto& w:myWatches) {
        if (w.myWatchID == wd) {
          FAMEvent e(w.myListener, w.myDirectory, event->name, event->mask);
          theEvents.push(e);
          break;
        }
      }
      i = i + sizeof(inotify_event) + event->len;
    }
  } while (theEvents.empty() || theEvents.size() != startSize);
} // FAMWatcher::getEvents

void
FAMWatcher::introduceSelf()
{
  std::shared_ptr<WatcherType> io = std::make_shared<FAMWatcher>();
  IOWatcher::introduce("fam", io);
}
