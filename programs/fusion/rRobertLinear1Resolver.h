#pragma once

#include "rVolumeValueResolver.h"

namespace rapio {
/* RobertLinear1Resolver
 *
 * One of my first attempts at making a value volume resolver, it tries
 * to do simple linear mapping.
 *
 * If you're interested in calculating values yourself, this acts as another
 * example, but the one matching w2merger the best is the LakResolver1
 * so I would skip this at first glance.
 *
 * There are also the example resolvers in the base/rVolumeValueResolver files.
 *
 * @author Robert Toomey
 */
class RobertLinear1Resolver : public VolumeValueResolver
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
