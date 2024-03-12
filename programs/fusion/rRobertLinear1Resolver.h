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

  /** Introduce into VolumeValueResolver factory */
  static void
  introduceSelf();

  /** Help function, subclasses return help information. */
  virtual std::string
  getHelpString(const std::string& fkey) override;

  /** Create by factory */
  virtual std::shared_ptr<VolumeValueResolver>
  create(const std::string & params) override;

  /** Calculate using VolumeValue inputs, our set of outputs in VolumeValue.*/
  virtual void
  calc(VolumeValue * vvp) override;

  /** We're going to use the weighted average */
  virtual std::shared_ptr<VolumeValue>
  getVolumeValue()
  {
    return std::make_shared<VolumeValueWeightAverage>();
  }

  /** Initialize a volume value IO for sending data.  Typically called by stage1 to
   * prepare to send the grid resolved values */
  virtual std::shared_ptr<VolumeValueIO>
  initForSend(
    const std::string   & radarName,
    const std::string   & typeName,
    const std::string   & units,
    const LLH           & center,
    size_t              xBase,
    size_t              yBase,
    std::vector<size_t> dims
  )
  {
    return std::make_shared<Stage2Data>(radarName, typeName, units, center, xBase, yBase, dims);
  }
};
}
