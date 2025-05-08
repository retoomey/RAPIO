#pragma once

#include "rVolumeValueResolver.h"
#include "rStage2Data.h"

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

  // ---------------------------------------------------
  // Functions for declaring available to command line
  //

  /** Introduce into VolumeValueResolver factory */
  static void
  introduceSelf();

  /** Help function, subclasses return help information. */
  virtual std::string
  getHelpString(const std::string& fkey) override;

  /** Create by factory */
  virtual std::shared_ptr<VolumeValueResolver>
  create(const std::string & params) override;

  // ---------------------------------------------------
  // Functions for declaring how to create/process data
  // for a stage2
  //

  /** We're going to use the weighted average */
  virtual std::shared_ptr<VolumeValue>
  getVolumeValue() override
  {
    return std::make_shared<VolumeValueWeightAverage>();
  }

  /** Return the VolumeValueOutputter class used for this resolver.  This allows resolvers to
   * write/read final output as they want. */
  virtual std::shared_ptr<VolumeValueIO>
  getVolumeValueIO() override
  {
    return std::make_shared<Stage2Data>();
  }

  /** Calculate using VolumeValue inputs, our set of outputs in VolumeValue.*/
  virtual void
  calc(VolumeValue * vvp) override;
};
}
