#include "rDataTypeHistory.h"
#include "rRadialSet.h"

// For the maximum history
// #include "rRAPIOAlgorithm.h"

#include <memory>

using namespace rapio;
using namespace std;

// Unified list of groups (Volumes, ProductGroups, etc.) that need time expiration
std::vector<std::shared_ptr<DataTypeGroup> > DataTypeHistory::myPurgeList;

void
DataTypeHistory::registerForPurging(std::shared_ptr<DataTypeGroup> group)
{
  if (group) {
    myPurgeList.push_back(group);
  }
}

void
DataTypeHistory::processNewData(RAPIOData& d)
{
  // NSEGridHistory::processNewData(d) used to be called here,
  // but environmental/NSE tracking has moved to local DataTypeGroups
  // in the algorithms (e.g. 3DVIL) that register themselves for purging.
}

void
DataTypeHistory::purgeTimeWindow(const Time& currentTime)
{
  // Notify all our histories of a time window event.
  // Time purge any volumes.  Volume named product containing N subtypes.
  // We're assuming data time in is the 'newest' in realtime and archive.
  // It's 'should' be true for archive if sorted, it's almost true
  // for realtime since different sources lag.
  for (auto& group : myPurgeList) {
    if (group) {
      group->purgeTimeWindow(currentTime);
    }
  }
}
