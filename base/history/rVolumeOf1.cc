#include "rVolumeOf1.h"
#include "rError.h"
#include <limits>

using namespace rapio;

void
VolumeOf1::addDataType(std::shared_ptr<DataType> dt)
{
  if (myItems.size() > 0) {
    const auto t    = myItems[0]->getTime();
    const auto tnew = dt->getTime();
    if (tnew >= t) {
      myItems[0] = dt;
    }
  } else {
    myItems.push_back(dt);
  }
  fLogInfo("{}", static_cast<const Volume&>(*this));
}

void
VolumeOf1::getTempPointerVector(VolumePointerCache& c)
{
  auto& levels = c.levels;
  auto& pc     = c.pc;

  pc.push_back(nullptr);
  pc.push_back(nullptr);

  if (myItems.size() > 0) {
    auto dt = myItems[0].get();
    pc.push_back(dt->getDataTypePointerCache().get());
    levels.push_back(std::numeric_limits<double>::max());
  }

  levels.push_back(std::numeric_limits<double>::max());
  pc.push_back(nullptr);
  pc.push_back(nullptr);
}
