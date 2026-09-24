#include "rVolumeOfN.h"
#include "rError.h"
#include "rStrings.h"
#include <limits>

using namespace rapio;

void
VolumeOfN::addDataType(std::shared_ptr<DataType> dt)
{
  const auto s = generateItemKey(dt);
  bool found   = false;

  for (auto it = myItems.begin(); it != myItems.end(); ++it) {
    const auto os = generateItemKey(*it);
    if (os == s) {
      *it   = dt;
      found = true;
      break;
    }
    if (os > s) {
      myItems.insert(it, dt);
      found = true;
      break;
    }
  }
  if (!found) {
    myItems.push_back(dt);
  }
}

void
VolumeOfN::getTempPointerVector(VolumePointerCache& c)
{
  auto& levels = c.levels;
  auto& pc     = c.pc;

  pc.push_back(nullptr);
  pc.push_back(nullptr);

  for (auto v : myItems) {
    auto dt = v.get();
    pc.push_back(dt->getDataTypePointerCache().get());

    const auto atCheck = generateItemKey(v);
    if (!atCheck.empty() && (atCheck[0] == 'a')) {
      levels.push_back(0.5);
      continue;
    }

    const auto os = Strings::removeNonNumber(atCheck);
    double d      = 0;
    try {
      d = std::stod(os);
    } catch (const std::exception& e) {
      fLogSevere("Subtype non number breaks volume of N: {}", atCheck);
    }
    levels.push_back(d);
  }

  levels.push_back(std::numeric_limits<double>::max());
  pc.push_back(nullptr);
  pc.push_back(nullptr);
}
