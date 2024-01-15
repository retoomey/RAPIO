#include "rElevationVolume.h"
#include "rRadialSet.h"
#include "rDataTypeHistory.h"
#include "rColorTerm.h"
#include "rStrings.h"

// For the maximum history
#include "rRAPIOAlgorithm.h"

#include <memory>

using namespace rapio;
using namespace std;

void
Volume::introduce(const std::string & key,
  std::shared_ptr<Volume>           factory)
{
  Factory<Volume>::introduce(key, factory);
}

void
Volume::introduceSelf()
{
  ElevationVolume::introduceSelf();
}

std::string
Volume::introduceHelp()
{
  std::string help;

  help += "Elevation volumes handle what tilts/levels are considered part of the current virtual volume.\n";
  auto e = Factory<Volume>::getAll();

  for (auto i: e) {
    help += " " + ColorTerm::fRed + i.first + ColorTerm::fNormal + " : " + i.second->getHelpString(i.first) + "\n";
  }
  return help;
}

void
Volume::introduceSuboptions(const std::string& name, RAPIOOptions& o)
{
  auto e = Factory<Volume>::getAll();

  for (auto i: e) {
    o.addSuboption(name, i.first, i.second->getHelpString(i.first));
  }
  // There's no 'non' volume
  // o.setEnforcedSuboptions(name, false);
}

std::string
ElevationVolume::getHelpString(const std::string& fkey)
{
  return "Stores all unique elevation angles within the history window (see help h).";
}

std::shared_ptr<Volume>
Volume::createVolume(
  const std::string & key,
  const std::string & params,
  const std::string & historyKey)
{
  auto f = Factory<Volume>::get(key);

  if (f == nullptr) {
    LogSevere("Couldn't create Volume from key '" << key << "', available: \n");
    auto e = Factory<Volume>::getAll();
    for (auto i: e) {
      LogSevere("  '" + i.first + "'\n"); // FIXME: help string later maybe
    }
  } else {
    // Pass onto the factory method
    f = f->create(historyKey, params);
    if (f != nullptr) {
      DataTypeHistory::registerVolume(historyKey, f); // We want time message for expiration
    }
  }
  return f;
}

std::shared_ptr<Volume>
Volume::createFromCommandLineOption(
  const std::string & option,
  const std::string & historyKey)
{
  std::string key, params;

  Strings::splitKeyParam(option, key, params);
  return createVolume(key, params, historyKey);
}

void
Volume::removeAt(size_t at)
{
  myVolume.erase(myVolume.begin() + at);
}

void
Volume::replaceAt(size_t at, std::shared_ptr<DataType> dt)
{
  myVolume[at] = dt;
}

void
Volume::insertAt(size_t at, std::shared_ptr<DataType> dt)
{
  myVolume.insert(myVolume.begin() + at, dt);
}

void
Volume::add(std::shared_ptr<DataType> dt)
{
  myVolume.push_back(dt);
}

void
Volume::purgeTimeWindow(const Time& time)
{
  // ----------------------------------------------------------------------------
  // Time purge from given time backwards
  for (size_t i = 0; i < myVolume.size(); i++) {
    // If datatype time is within the history window from the current time passed in
    if (!RAPIOAlgorithm::inTimeWindow(myVolume[i]->getTime())) {
      removeAt(i); //  myVolume.erase(myVolume.begin() + i);
      i--;
    }
  }
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
      replaceAt(i, dt); // myVolume[i] = dt;
      found = true;
      break;
    }

    // if the current index is greater than our subtype insert before
    if (os > s) {      // FIXME: Doing this by string.  Does this always work?  Seems to for our stuff
      insertAt(i, dt); // myVolume.insert(myVolume.begin() + i, dt);
      found = true;
      break;
    }
  }
  if (!found) {
    add(dt); // myVolume.push_back(dt);
  }

  // For moment print out volume
  // printVolume();
} // Volume::addDataType

void
Volume::getTempPointerVector(std::vector<double>& levels, std::vector<DataType *>& pointers,
  std::vector<DataProjection *>& projectors)
{
  // Padding for search loop so range end checking not needed, which speeds things up.  See the
  // getSpreadL loop in ElevationVolume.
  // FIXME: Need to cleanup/revisit the normal spread functions since not used
  pointers.push_back(nullptr);
  pointers.push_back(nullptr);
  projectors.push_back(nullptr);
  projectors.push_back(nullptr);
  for (auto v:myVolume) {
    // Cached pointers for speed.
    pointers.push_back(v.get());
    projectors.push_back(v.get()->getProjection().get());
    const auto os = v->getSubType();
    double d      = std::stod(os); // FIXME catch?
    levels.push_back(d);
  }
  levels.push_back(std::numeric_limits<double>::max());
  // End padding
  pointers.push_back(nullptr);
  pointers.push_back(nullptr);
  projectors.push_back(nullptr);
  projectors.push_back(nullptr);
}

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
      removeAt(i); // myVolume.erase(myVolume.begin() + i);
      return true;
    }
  }
  return false;
}

void
ElevationVolume::introduceSelf()
{
  std::shared_ptr<ElevationVolume> newOne = std::make_shared<ElevationVolume>();
  Factory<Volume>::introduce("simple", newOne);
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

std::ostream&
rapio::operator << (std::ostream& os, const Volume& v)
{
  std::stringstream z;

  Time latest(0);
  std::vector<double> out1;
  std::vector<Time> times;

  z << "Current Virtual Volume: ";
  for (auto x:v.getVolume()) {
    const auto os = x->getSubType();
    double d      = std::stod(os);
    Time newer    = x->getTime();
    if (newer > latest) {
      latest = newer;
    }
    out1.push_back(d);
    times.push_back(newer);
  }

  for (size_t c = 0; c < out1.size(); c++) {
    z << out1[c];
    if (times[c] == latest) {
      z << " (latest)";
    }
    z << ", ";
  }
  return (os << z.str());
}
