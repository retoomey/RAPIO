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
  // printVolume();
} // Volume::addDataType

std::vector<double>
Volume::getNumberVector()
{
  // Convert each subtype to an int for doing a fast lookup
  // using an int because later search will be quicker
  std::vector<double> numbers;

  for (auto v:myVolume) {
    const auto os = v->getSubType();
    double d      = std::stod(os); // FIXME catch?
    numbers.push_back(d);
  }
  return numbers;
}

void
Volume::getSpread(float at, const std::vector<double>& numbers, DataType *& lower, DataType *& upper)
{
  /* std::cout << "Incoming ranges looking for " << at << "\n";
   * for (auto v:numbers){
   *  std::cout << v << ", ";
   * }
   * std::cout << "\n";
   */

  // Binary search
  lower = upper = nullptr;
  int left  = 0;
  int right = numbers.size() - 1;
  int found = -100;

  while (left <= right) {
    const int midv = left + (right - left) / 2;
    const auto v   = numbers[midv];
    if (v == at) {
      found = midv;
      break;
    } else if (v < at) {
      left = midv + 1;
    } else {
      right = midv - 1;
    }
  }

  // Direct hit, use mid index
  if (found > -100) {
    lower = myVolume[found].get();
    if (found + 1 < numbers.size() - 1) {
      upper = myVolume[found + 1].get();
    }
  } else {
    // So left is now <= right. Pin to the bins
    if (left > numbers.size() - 1) { left = numbers.size() - 1; }
    if (right < 0) { right = 0; }

    const float l = numbers[right]; // switch em back
    const float r = numbers[left];

    // Same left/right special cases
    if (left == right) { // left size or right size
      if (at >= r) {     // past range, use top
        lower = myVolume[left].get();
        upper = nullptr;
      } else if (at <= l) { // before range, use bottom
        upper = myVolume[right].get();
        lower = nullptr;
      }
    } else {                           // left and right are different
      if ((l <= at) && ( at <= r)) {   // between ranges
        lower = myVolume[right].get(); // left/right swapped
        upper = myVolume[left].get();
      }
    }
  }
  // std::cout << "Hit, left and right... " << found << " " << left << " and " << right << "\n";

  #if 0
  // binary search.
  size_t max = numbers.size() - 1;
  std::cout << "Incoming length is " << max << "\n";
  int high = max;
  int low  = 0;
  std::cout << "Low and high " << low << ", " << high << " searching for " << at << "\n";
  while (true) {
    int mid = (low + high) / 2;
    if (at == numbers[mid]) { // direct hit
      std::cout << "Found " << at << " at " << mid << "\n";
      lower = myVolume[mid].get(); // If direct hit use lower
      if (mid < max) {             // upper only if exists
        upper = myVolume[mid + 1].get();
      } else {
        upper = nullptr;
      }
      return;
    } else if (at > numbers[mid]) {
      low = mid + 1;
      std::cout << "Low becomes [" << low << ", " << high << "]\n";
    } else {
      high = mid - 1;
      std::cout << "High becomes [" << low << ", " << high << "]\n";
    }
    if (low >= high) {
      int use = (low > high) ? high : low;

      std::cout << "End hits at " << low << " , " << high << "\n";

      lower = myVolume[use].get(); //  We could be lower than lowest as well
      if (at >= numbers[use]) {    // if less than outside the 'bin' just use the lower and down interpolate
        if (low + 1 <= max) {      // If not at the 'end' than we have one above
          upper = myVolume[use + 1].get();
        } else {
          upper = nullptr;
        }
      } else {
        upper = nullptr;
        // Below the hit field (say index 0) we use that lower
      }
      std::cout << "FINAL " << (void *) (lower) << " to " << (void *) (upper) << "\n";

      /*
       * if (low
       * if (at <= numbers[low]){ // if value less or equal, use only one
       * lower= myVolume[mid].get();  // If direct hit use lower
       * upper = nullptr;
       * }else{
       * }
       */

      return;
    }
  }

  #endif // if 0
} // Volume::getSpread

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
