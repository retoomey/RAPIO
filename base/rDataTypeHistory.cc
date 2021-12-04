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

void
DataTypeHistory::addRecord(Record& rec)
{
  // History is stored by an ordered tree of selections.
  // Records internal to the tree would be inefficient, so we convert
  // to a new data structure here.
  LogSevere("Adding record: \n");
  // for(auto& p:rec.getBuilderParams()){
  //  std::cout << "   " << p << "\n";
  // }
  auto& sels   = rec.getSelections();
  size_t aSize = sels.size();

  // There should always be at least three selections
  if (aSize <= 3) {
    // The last selection is assumed to be the time string, we don't store that
    // We 'could' do maps of maps but that would be so memory intensive...

    // Index number is const for a particular algorithm run..
    // This corresponds to the 'source' for the moment.  We need a 'name'
    // for a source to refer to it actually. The display does this with a name
    // to index location lookup which as a direct algorithm we don't have currently.
    const size_t i = rec.getIndexNumber();
    std::cout << "Index in is " << i << "\n";

    for (int i = aSize - 2; i >= 0; i--) {
      std::cout << "   " << sels[i] << "\n";
    }
  } else {
    LogSevere("Trying to add record to history with a selections size of " << aSize << " which is unknown.\n");
  }
}
