#pragma once

#include "rVolumeValueResolver.h"

namespace rapio {
/**
 * A resolver attempting to use the math presented in the paper:
 * "A Real-Time, Three-Dimensional, Rapidly Updating, Heterogeneous Radar
 * Merger Technique for Reflectivity, Velocity, and Derived Products"
 * Weather and Forecasting October 2006
 * Valliappa Lakshmanan, Travis Smith, Kurt Hondl, Greg Stumpf
 *
 * In particular, page 10 describing Virtual Volumes and the elevation
 * influence.
 */
class LakResolver1 : public VolumeValueResolver
{
public:

  /** Introduce into VolumeValueResolver factory */
  static void
  introduceSelf();

  /** Create by factory */
  virtual std::shared_ptr<VolumeValueResolver>
  create(const std::string & params);

  virtual void
  calc(VolumeValue& vv) override;
};
}
