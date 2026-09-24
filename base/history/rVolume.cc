#include "rVolume.h"
#include "rVolumeOf1.h"
#include "rVolumeOfN.h"
#include "rDataTypeHistory.h"
#include "rColorTerm.h"
#include "rStrings.h"

using namespace rapio;
using namespace std;

void
Volume::introduce(const std::string & key, std::shared_ptr<Volume> factory)
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
}

std::shared_ptr<Volume>
Volume::createVolume(
  const std::string & key,
  const std::string & params,
  const std::string & historyKey)
{
  auto f = Factory<Volume>::get(key);

  if (f == nullptr) {
    fLogSevere("Couldn't create Volume from key '{}', available: ", key);
    auto e = Factory<Volume>::getAll();
    for (auto i: e) {
      fLogSevere("  '{}'", i.first);
    }
  } else {
    f = f->create(historyKey, params);
    if (f != nullptr) {
      DataTypeHistory::registerForPurging(f);
    }
  }
  return f;
}

std::ostream&
rapio::operator << (std::ostream& os, const Volume& v)
{
  return os << fmt::format("{}", v);
}
