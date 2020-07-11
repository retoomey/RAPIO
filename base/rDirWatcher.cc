#include "rDirWatcher.h"

#include "rError.h" // for LogInfo()
#include "rOS.h"

#include <dirent.h>
#include <sys/stat.h>

#include <queue>

using namespace rapio;

namespace
{
/** Time filter test for file/directory stats */
bool
greaterStat(struct stat& a, struct stat& b)
{
  if (a.st_ctim.tv_sec > b.st_ctim.tv_sec) {
    return true;
  } else if (a.st_ctim.tv_sec == b.st_ctim.tv_sec) {
    if (a.st_ctim.tv_nsec > b.st_ctim.tv_nsec) {
      return true;
    }
  }
  return false;
}
}

void
DirWatcher::scan(IOListener * listener, const std::string& dir, struct stat& lowtime, struct stat& newlowtime)
{
  // Open directory
  DIR * dirp = opendir(dir.c_str());

  if (dirp == 0) {
    LogSevere("Unable to read location " << dir << "\n");
    // Do a unmount event? return (false);
  }
  struct dirent * dp;
  while ((dp = readdir(dirp)) != 0) {
    const std::string full = dir + "/" + dp->d_name;

    // Always ignore files starting with . (0 terminated so don't need length check)
    if (dp->d_name[0] == '.') { continue; }

    // Stat for time...note this allows us to skip old directories too
    // Does it break the low time threshhold?
    struct stat dst;
    if (stat(full.c_str(), &dst) != 0) {
      // What to do if stat is failing?
      LogSevere("Stat is failing, unable to poll directory.\n");
      return;
    }
    const auto nanos    = dst.st_ctim.tv_nsec;
    const auto secs     = dst.st_ctim.tv_sec;
    const bool overTime = greaterStat(dst, lowtime);

    if (overTime) {
      // Recursive go into directories
      if (OS::isDirectory(full)) {
        scan(listener, full, lowtime, newlowtime);
      } else {
        // FIXME: Push into queue if wanted... realtime mode maybe so a skip pass at beginning
        // LogSevere("    NEW:" << full << " " << newlowtime.st_ctim.tv_sec << "." << newlowtime.st_ctim.tv_nsec << "\n");
        WatchEvent e(listener, "newfile", full);
        myEvents.push(e);
      }

      // Keep the max time of any of the new files, this will be new low threshold
      if (greaterStat(dst, newlowtime)) {
        newlowtime.st_ctim.tv_sec  = secs;
        newlowtime.st_ctim.tv_nsec = nanos;
      }
    } else {
      //     LogSevere("    Under:"<<full<< " " <<secs <<"." << nanos << "\n");
    }
  }

  closedir(dirp);
} // doit

void
DirWatcher::DirInfo::createEvents(WatcherType * w)
{
  std::string loc = myURL.toString();
  // Check local url..

  // Latest low threshhold filter

  struct stat newLast = myLastStat;
  // auto lastnanos = w.myLastStat.st_ctim.tv_nsec;
  // auto lastsecs = w.myLastStat.st_ctim.tv_sec;
  // LogSevere("CURRENT LOW: " << lastsecs <<"."<<lastnanos<<"\n");

  // Get up to a max number of new files.  This allows
  // multiple directory watchers to share bandwidth and not hog
  // the system.
  ((DirWatcher *) w)->scan(myListener, loc, myLastStat, newLast);
  myLastStat = newLast;
}

void
DirWatcher::getEvents()
{
  for (auto& w:myWatches) {
    w->createEvents(this);
  }
} // DirWatcher::getEvents

bool
DirWatcher::attach(const std::string & dirname,
  IOListener *                       l)
{
  std::shared_ptr<DirInfo> newWatch = std::make_shared<DirInfo>(l, dirname);
  myWatches.push_back(newWatch);

  return true;
}

void
DirWatcher::introduceSelf()
{
  std::shared_ptr<DirWatcher> io = std::make_shared<DirWatcher>();
  IOWatcher::introduce("dir", io);
}
