#include "rElevationVolume.h"
#include "rRadialSet.h"

// For the maximum history
#include "rRAPIOAlgorithm.h"

#include <memory>

using namespace rapio;
using namespace std;

void
Volume::purgeTimeWindow(const Time& time)
{
  // ----------------------------------------------------------------------------
  // Time purge from given time backwards
  for (size_t i = 0; i < myVolume.size(); i++) {
    // if (!RAPIOAlgorithm::inTimeWindow(dt->getTime()+TimeDuration(-10000))) // Make it older than 15 mins (900 seconds)
    if (!RAPIOAlgorithm::inTimeWindow(time)) {
      // LogInfo(myVolume[i]->getSubType() << " --PURGE-- " << myVolume[i]->getTime() << "\n");
      myVolume.erase(myVolume.begin() + i);
      i--;
    }
  }
}

void
Volume::printVolume()
{
  LogInfo("------CURRENT VOLUME LIST " << myKey << "(" << (void *) (this) << ") --------\n");
  for (auto x:myVolume) {
    LogInfo(x->getSubType() << " Time: " << x->getTime() << " " << myKey << "\n");
  }
  LogInfo("------END CURRENT VOLUME LIST ----------------------------------------\n");
}

void
Volume::addDataType(std::shared_ptr<DataType> dt)
{
  // Map lookup key for volume name is DataType+TypeName such as "RadialSet-Reflectivity"
  // Each of these volumes is independent from others
  // auto dataType = dt->getDataType(); // RadialSet, LatLonGrid
  // auto typeName = dt->getTypeName(); // Reflectivity

  // Sort key is the subtype such as Elevation (RadialSet), Height (LatLonGrid)
  const auto s = dt->getSubType();

  // ----------------------------------------------------------------------------
  // Insertion sort by subtype.  This allows quick data lookup
  // FIXME: Maybe convert to a number to speed up when searching
  //
  bool found = false;

  for (size_t i = 0; i < myVolume.size(); i++) {
    const auto os = myVolume[i]->getSubType();

    // If equal subtype, replace and we're done...
    if (os == s) {
      myVolume[i] = dt;
      found       = true;
      break;
    }

    // if the current index is greater than our subtype insert before
    if (os > s) { // FIXME: Doing this by string.  Does this always work?  Seems to for our stuff
      myVolume.insert(myVolume.begin() + i, dt);
      found = true;
      break;
    }
  }
  if (!found) {
    myVolume.push_back(dt);
  }

  // For moment print out volume
  printVolume();
} // Volume::addDataType

std::shared_ptr<DataType>
Volume::getSubType(const std::string& subtype)
{
  for (auto v:myVolume) {
    if (v->getSubType() == subtype) { return v; }
  }
  return nullptr;
}

bool
Volume::deleteSubType(const std::string& subtype)
{
  for (size_t i = 0; i < myVolume.size(); ++i) {
    if (myVolume[i]->getSubType() == subtype) {
      myVolume.erase(myVolume.begin() + i);
      return true;
    }
  }
  return false;
}

void
ElevationVolume::addDataType(std::shared_ptr<DataType> dt)
{
  // For elevation check it's a RadialSet, do any 'extra' required
  std::shared_ptr<RadialSet> rs = std::dynamic_pointer_cast<RadialSet>(dt);

  if (rs != nullptr) {
    Volume::addDataType(dt);
  }
}
