#include "rLak2DResolver.h"

using namespace rapio;

void
Lak2DResolver::introduceSelf()
{
  std::shared_ptr<Lak2DResolver> newOne = std::make_shared<Lak2DResolver>();
  Factory<VolumeValueResolver>::introduce("lak2D", newOne);
}

std::string
Lak2DResolver::getHelpString(const std::string& fkey)
{
  return "Based on w2merger 2D ground projection.";
}

std::shared_ptr<VolumeValueResolver>
Lak2DResolver::create(const std::string & params)
{
  return std::make_shared<Lak2DResolver>();
}

void
Lak2DResolver::calc(VolumeValue * vvp)
{
  // Count each value if it contributes, if masks are covered than make missing
  auto& vv = *(VolumeValueWeightAverage *) (vvp);

  // ------------------------------------------------------------------------------
  // Query information for the location
  // For the 'one' volume type, we have a single upper layer always (if we have data)
  // FIXME: If this works we might make API more obvious.  This is because the
  // search is set up to hit immediately, giving us null null, tilt, null for the layers.
  // Basically our virtual subtype angle is always < max.
  if (queryUpper(vv)) {
    const double& value = vv.getUpperValue().value;
    if (Constants::isGood(value)) {
      // This weight is for merging with other radars...so still need it.
      // Well we need it for 2D distance merging, for max it will be cancelled out.
      const auto rw   = 1.0 / std::pow(vv.virtualRangeKMs, 2); // inverse square
      const double aV = value;                                 // Only hit value counts, no vertical interpolation/weighting
      vv.dataValue = aV;
      vv.topSum    = rw * aV; // Stage2 just makes v = topSum/bottomSum
      vv.bottomSum = rw;
    } else {
      // Not a good data value, but we hit the product, so missing.
      vv.dataValue = Constants::MissingData;
      vv.topSum    = vv.dataValue;
      vv.bottomSum = 1.0;
    }
  } else {
    // No data, unavailable
    vv.dataValue = Constants::DataUnavailable;
    vv.topSum    = vv.dataValue;
    vv.bottomSum = 1.0;
  }
} // calc
