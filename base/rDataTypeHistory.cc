#include "rDataTypeHistory.h"
#include "rRadialSet.h"

// For the maximum history
// #include "rRAPIOAlgorithm.h"

#include <memory>

using namespace rapio;
using namespace std;

std::map<std::string, std::shared_ptr<Volume> > DataTypeHistory::myVolumes;

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
  LogInfo("Adding DataType to history volume cache: " << key << "\n");

  // Volume
  std::shared_ptr<Volume> v;
  auto lookup = myVolumes.find(key);
  if (lookup == myVolumes.end()) {
    // Ahh bleh we need to differentiate by type.
    // FIXME: Probably get the volume subtype from the datatype
    std::shared_ptr<RadialSet> rs = std::dynamic_pointer_cast<RadialSet>(data);
    if (rs != nullptr) {
      v = std::make_shared<ElevationVolume>(key); // RadialSets for now
    } else {
      v = std::make_shared<Volume>(key); // LatLonGrids for now
    }
    myVolumes[key] = v;
  } else {
    v = lookup->second;
  }
  v->addDataType(data);
}

void
DataTypeHistory::purgeTimeWindow(const Time& currentTime)
{
  // Time purge any volumes.  Volume named product containing N subtypes.
  // We're assuming data time in is the 'newest' in realtime and archive.
  // It's 'should' be true for archive if sorted, it's almost true
  // for realtime since different sources lag.
  for (auto s:myVolumes) {
    s.second->purgeTimeWindow(currentTime);
  }
}
