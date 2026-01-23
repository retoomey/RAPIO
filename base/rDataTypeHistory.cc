#include "rDataTypeHistory.h"
#include "rRadialSet.h"

// For the maximum history
// #include "rRAPIOAlgorithm.h"

#include <memory>

using namespace rapio;
using namespace std;

std::map<std::string, std::shared_ptr<Volume> > VolumeHistory::myVolumes;

// NSE Grid static stuff.  FIXME: introduce classes
std::vector<std::string> NSEGridHistory::myKeys;

std::vector<std::string> NSEGridHistory::myDataTypeNames;

std::vector<std::shared_ptr<DataType> > NSEGridHistory::myDataTypeCache;

std::vector<float> NSEGridHistory::myDefaultValues;

void
VolumeHistory::registerVolume(const std::string& key, std::shared_ptr<Volume> v)
{
  myVolumes[key] = v;
}

void
DataTypeHistory::processNewData(RAPIOData& d)
{
  NSEGridHistory::processNewData(d);
}

#if 0
void
DataTypeHistory::updateVolume(std::shared_ptr<DataType> data)
{
  // Map lookup key for volume name is DataType+TypeName such as "RadialSet-Reflectivity"
  // Each of these volumes is independent from others
  auto dataType = data->getDataType(); // RadialSet, LatLonGrid
  auto typeName = data->getTypeName(); // Reflectivity

  std::string radar = "None";

  data->getString("radarName-value", radar);

  // KTLX_Reflectivity_RadialSet for RadialSets
  std::string key = radar + '_' + typeName + '_' + dataType;

  fLogInfo("Adding DataType to history volume cache: {}", key);

  // Volume
  std::shared_ptr<Volume> v;
  auto lookup = myVolumes.find(key);

  if (lookup == myVolumes.end()) {
    v = std::make_shared<VolumeOfN>(key);
    registerVolume(key, v);
  } else {
    v = lookup->second;
  }
  v->addDataType(data);
} // DataTypeHistory::updateVolume

std::vector<std::shared_ptr<DataType> >
DataTypeHistory::getFullSubtypeGroup(
  const std::string             & subtype,
  const std::vector<std::string>& keys)
{
  // Hunt volumes for the keys?
  std::vector<std::shared_ptr<DataType> > out;
  size_t counter = 0;

  for (auto v:myVolumes) {
    auto dt = v.second->getSubType(subtype);
    out.push_back(dt);
    if (dt != nullptr) { // We want FULL group
      counter++;
    }
  }
  if (counter == keys.size()) {
    return out;
  }
  return std::vector<std::shared_ptr<DataType> >();
}

size_t
DataTypeHistory::deleteSubtypeGroup(
  const std::string             & subtype,
  const std::vector<std::string>& keys)
{
  size_t counter = 0;

  for (auto v:myVolumes) {
    if (v.second->deleteSubType(subtype)) {
      counter++;
    }
  }
  return counter;
}

#endif // if 0

void
DataTypeHistory::purgeTimeWindow(const Time& currentTime)
{
  // Notify all our histories of a time window event.
  VolumeHistory::purgeTimeWindow(currentTime);
  // NSE history?
}

void
VolumeHistory::purgeTimeWindow(const Time& currentTime)
{
  // Time purge any volumes.  Volume named product containing N subtypes.
  // We're assuming data time in is the 'newest' in realtime and archive.
  // It's 'should' be true for archive if sorted, it's almost true
  // for realtime since different sources lag.
  for (auto s:myVolumes) {
    s.second->purgeTimeWindow(currentTime);
  }
}

int
NSEGridHistory::followGrid(const std::string& key, const std::string& aTypeName, float defaultValue)
{
  // If already following, return the index (Note we only add, never remove)
  for (size_t i = 0; i < myKeys.size(); ++i) {
    if (myKeys[i] == key) {
      return i;
    }
  }

  // FIXME: Might be better to encapsulate into a class to avoid errors
  myKeys.push_back(key);
  myDataTypeNames.push_back(aTypeName);
  myDataTypeCache.push_back(nullptr);
  myDefaultValues.push_back(defaultValue);
  fLogInfo("Following NSE: '{}' with typename '{}' and default value of {}",
    key, aTypeName, defaultValue);
  return myKeys.size() - 1;
}

std::shared_ptr<DataType>
NSEGridHistory:: getGrid(const std::string& aTypeName)
{
  // Uggh MRMS remaps and scales to the conus?
  // I'm not sure I like this.  I'd rather query by lat/lon but that could
  // also make porting algs more difficult.
  return nullptr;
}

void
NSEGridHistory::processNewData(RAPIOData& d)
{
  // See if DataType matches our follow list...then we create the DataType
  // and cache it for later.
  const std::string& dt = d.record().getDataType();
  bool found = false;

  for (size_t i = 0; i < myDataTypeNames.size(); ++i) {
    if (myDataTypeNames[i] == dt) {
      fLogInfo("NSE received {}, caching.", dt);
      myDataTypeCache[i] = d.datatype<DataType>();
      found = true;
      break;
    }
  }

  if (found) {
    // FIXME: Do we need to remap or anything at this moment?
  }
}
