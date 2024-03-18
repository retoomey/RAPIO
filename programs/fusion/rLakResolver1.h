#pragma once

#include "rVolumeValueResolver.h"
#include "rStage2Data.h"

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
