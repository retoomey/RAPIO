#pragma once

#include "rVolumeValueResolver.h"
#include "rStage2Data.h"

namespace rapio {
/** Grid cell output storage for our resolver */
class VolumeValueVelGatherer : public VolumeValue
{
public:
  float ux;
  float uy;
  float uz;
  float latDegs;
  float lonDegs;
  float heightKMs;
};

/**
 * A resolver attempting to gather data for a velocity collection
 * attempt.
 *
 * @author Robert Toomey
 */
class VelResolver : public VolumeValueResolver
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

  /** We're going to use our custom gatherer */
  virtual std::shared_ptr<VolumeValue>
  getVolumeValue() override
  {
    return std::make_shared<VolumeValueVelGatherer>();
  }

  /** Return the VolumeValueOutputter class used for this resolver.  This allows resolvers to
   * write/read final output as they want. */
  virtual std::shared_ptr<VolumeValueIO>
  getVolumeValueIO() override
  {
    // FIXME: We're gonna a different class
    return std::make_shared<Stage2Data>();
  }

  /** Calculate using VolumeValue inputs, our set of outputs in VolumeValue.*/
  virtual void
  calc(VolumeValue * vvp) override;
};
}
