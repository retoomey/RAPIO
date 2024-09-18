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
  VolumeOfN::introduceSelf();
  VolumeOf1::introduceSelf();
}

std::string
Volume::introduceHelp()
{
  std::string help;

  help += "Volumes handle a collection of received DataTypes usually some sort of virtual volume.\n";
  help += "Usually these are time purged based on the history window (see help h).\n";
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
      VolumeHistory::registerVolume(historyKey, f); // We want time message for expiration
    }
  }
  return f;
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
VolumeOfN::addDataType(std::shared_ptr<DataType> dt)
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

  // LogInfo(*this << "\n");
} // Volume::addDataType

void
VolumeOf1::addDataType(std::shared_ptr<DataType> dt)
{
  if (myVolume.size() > 0) {
    // Replace our single item, iff the time is newer
    const auto t    = myVolume[0]->getTime();
    const auto tnew = dt->getTime();
    if (tnew >= t) {
      replaceAt(0, dt);
    }
  } else {
    add(dt); // Add what we got
  }
  LogInfo(*this << "\n");
}

void
VolumeOfN::getTempPointerVector(std::vector<double>& levels, std::vector<DataType *>& pointers,
  std::vector<DataProjection *>& projectors)
{
  // Padding for search loop so range end checking not needed, which speeds things up.
  pointers.push_back(nullptr);
  pointers.push_back(nullptr);
  projectors.push_back(nullptr);
  projectors.push_back(nullptr);
  for (auto v:myVolume) {
    // Cached pointers for speed.
    pointers.push_back(v.get());
    projectors.push_back(v.get()->getProjection().get());

    // Hack attempt for "at" overloaded meaning (non-angle)
    const auto atCheck = v->getSubType();
    if (!atCheck.empty() && (atCheck[0] == 'a')) {
      levels.push_back(0.5);
      continue;
    }

    // Push back the subtype used for the getSpreadL search
    const auto os = Strings::removeNonNumber(v->getSubType());
    double d      = 0;
    try {
      d = std::stod(os);
    }catch (const std::exception& e) {
      // FIXME: How to handle this data error?
      LogSevere("Subtype non number breaks volume of N: " << v->getSubType() << "\n");
    }
    levels.push_back(d);
  }
  // End padding
  levels.push_back(std::numeric_limits<double>::max());
  pointers.push_back(nullptr);
  pointers.push_back(nullptr);
  projectors.push_back(nullptr);
  projectors.push_back(nullptr);
} // VolumeOfN::getTempPointerVector

void
VolumeOf1::getTempPointerVector(std::vector<double>& levels, std::vector<DataType *>& pointers,
  std::vector<DataProjection *>& projectors)
{
  // We're gonna trick the getSpreadL by simply storing a max number first subtype.
  // This means the resolver will always get a 'upper' tilt hit.
  // Padding for search loop so range end checking not needed, which speeds things up.
  pointers.push_back(nullptr);
  pointers.push_back(nullptr);
  projectors.push_back(nullptr);
  projectors.push_back(nullptr);
  if (myVolume.size() > 0) {
    // Push back the DataType and its projection as cached pointers for speed
    pointers.push_back(myVolume[0].get());
    projectors.push_back(myVolume[0].get()->getProjection().get());

    // Ignore subtype, we want the getSpreadL search to always instantly hit.  So push a max
    // which will cause a < search auto hit
    levels.push_back(std::numeric_limits<double>::max());
  }
  // End padding
  levels.push_back(std::numeric_limits<double>::max());
  pointers.push_back(nullptr);
  pointers.push_back(nullptr); // Might not need this one
  projectors.push_back(nullptr);
  projectors.push_back(nullptr); // Might not need this one
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

std::ostream&
rapio::operator << (std::ostream& os, const Volume& v)
{
  std::stringstream z;

  Time latest(0);
  std::vector<double> out1;
  std::vector<Time> times;

  z << "Current Virtual Volume: ";
  for (auto x:v.getVolume()) {
    const auto os = Strings::removeNonNumber(x->getSubType());
    double d      = 0;
    try {
      d = std::stod(os);
    }catch (const std::exception& e) {
      // FIXME: How to handle this data error?
      // LogSevere("Subtype non number breaks volume of N: " << v->getSubType() << "\n");
    }

    Time newer = x->getTime();
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
} // <<
