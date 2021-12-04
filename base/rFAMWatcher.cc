#include "rFAMWatcher.h"

#include "rError.h"
#include "rOS.h"
#include "rIOURL.h"
#include "rFileIndex.h"

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/inotify.h>

#include <queue>
#include <algorithm>

#include <dirent.h>

using namespace rapio;

/** Global file instance for inotify */
int FAMWatcher::theFAMID = -1;

/** Default constant for a fam watcher */
const std::string FAMWatcher::FAM_WATCH = "fam";

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

bool
FAMWatcher::attach(FAMInfo * w)
{
  /** Check if initial inotify creation worked */
  if (theFAMID < 0) {
    init();
    if (theFAMID < 0) { return (false); }
  }

  // Mask for FAM events
  uint32_t mask = IN_CLOSE_WRITE | IN_MOVED_TO | IN_ONLYDIR |

    // Watched directory itself was deleted.  Important to reconnect
    // or kill algorithm since we just lost connection.
    IN_DELETE_SELF |

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

  int watchID = inotify_add_watch(theFAMID, w->myDirectory.c_str(), mask);
  if (watchID < 0) {
    std::string errMessage(strerror(errno));
    LogSevere(
      "FAM attempt: " << w->myDirectory << ": " << errMessage
                      << "\n");
    return false;
  } else {
    LogInfo("FAM Attached to " << w->myDirectory << "\n");
    w->myWatchID = watchID;
    w->myTime    = Time::CurrentTime();
  }
  return true;
} // FAMWatcher::attach

bool
FAMWatcher::attach(const std::string &dirname,
  bool                               realtime,
  bool                               archive,
  IOListener *                       l)
{
  // On archive do initial scan of directory for any existing files
  if (archive) {
    DIR * dirp = opendir(dirname.c_str());
    if (dirp == 0) {
      LogSevere("Unable to read location " << dirname << "\n");
      return (false);
    }
    struct dirent * dp;
    while ((dp = readdir(dirp)) != 0) {
      // Humm adding string here might be wasteful...
      const std::string full = dirname + "/" + dp->d_name;
      if (!OS::isDirectory(full)) {
        ((FileIndex *) l)->handleFile(full);
      }
    }
    closedir(dirp);
  }

  if (realtime) {
    std::shared_ptr<FAMInfo> newWatch = std::make_shared<FAMInfo>(l, dirname, -1);
    if (!attach(newWatch.get())) { // Failed initial attachment
      // If not auto connect mode, die (no watch)
      if (!myAutoReconnect) {
        return false;
      }
    }
    // Push broken or successful watch for checking
    myWatches.push_back(newWatch);
    LogDebug(dirname << " being monitored ... \n");
  }
  return true;
} // FAMWatcher::attach

void
FAMWatcher::addFAMEvent(FAMInfo * w, const inotify_event * event)
{
  IOListener * l = w->myListener;
  const std::string& directory = w->myDirectory;
  const uint32_t amask         = event->mask;

  bool deleted = false;

  // The watch was automatically removed, because the file was deleted or its
  // filesystem was unmounted.
  // I think you can't get this without getting the unmount or self delete first..
  if (amask & IN_IGNORED) {
    // LogSevere("IGNORED EVENT "<< w->myWatchID <<"\n");
    w->myWatchID = -1;
    deleted      = true;
  }

  // So we don't actually need to check unmount or delete self, since
  // these will cause FAM to self delete the watch

  // The backing filesystem was unmounted
  // This is usually followed by a IN_IGNORED (watch auto remove) ?? FIXME: yes/no?
  if (amask & IN_UNMOUNT) {
    // LogSevere("UNMOUNT EVENT\n");
    deleted = false;// wait for IN_IGNORED
  }

  // Watched file/directory was itself deleted
  // This is usually followed by a IN_IGNORED (watch auto remove) yes
  if (amask & IN_DELETE_SELF) {
    // LogSevere("DELETE SELF EVENT\n");
    deleted = false; // wait for IN_IGNORED
  }

  if (deleted) {
    WatchEvent e(l, myAutoReconnect ? "unmountr" : "unmount", directory);
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
      const std::string& shortfile = event->name;
      WatchEvent e(l, "newfile", directory + '/' + shortfile);
      myEvents.push(e);
    }
  }
} // FAMWatcher::addFAMEvent

void
FAMWatcher::getEvents()
{
  // Ignore global FAM polling if no watches left or exist
  if (myWatches.size() == 0) {
    return;
  }

  // Check for disconnected watches and try to reconnect them...
  // This happens if directory deleted, file system unmounted,
  // or if directory didn't exist yet, which is 'possible' in realtime
  // system.
  if (myAutoReconnect) {
    Time now = Time::CurrentTime();
    for (auto ww:myWatches) {
      auto * w = (FAMInfo *) (ww.get());
      if (w->myWatchID < 0) { // Basically if disconnected check
        TimeDuration d(now - w->myTime);
        if (d.seconds() > myAutoSeconds) { // and enough time passed to not hammer
          w->myTime = now;
          attach(w);
        }
      }
    }
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
      const inotify_event * event = (inotify_event *) (&(buf[i]));

      const int wd = event->wd;

      // This is O(n) but ends up being faster than std::map in practice
      for (auto ww:myWatches) {
        auto * w = (FAMInfo *) (ww.get());
        if (w->myWatchID == wd) {
          addFAMEvent(w, event);
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

  IOWatcher::introduce(FAM_WATCH, io);
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
