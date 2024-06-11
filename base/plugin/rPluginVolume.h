#pragma once

#include "rRAPIOPlugin.h"
#include <rElevationVolume.h>

namespace rapio {
class RAPIOProgram;
class RAPIOOptions;

/** Wrap the Volume in the newer generic plugin model.
 * Possibly we could cleanup/refactor a bit...for now we wrap the
 * global Volume class. */
class PluginVolume : public RAPIOPlugin {
public:

  /** Create a Volume plugin */
  PluginVolume(const std::string& name) : RAPIOPlugin(name){ }

  /** Declare plugin. */
  static bool
  declare(RAPIOProgram * owner, const std::string& name = "volume");

  /** Declare options for the plugin */
  virtual void
  declareOptions(RAPIOOptions& o) override;

  /** Declare advanced help for the plugin */
  virtual void
  addPostLoadedHelp(RAPIOOptions& o) override;

  /** Process our options */
  virtual void
  processOptions(RAPIOOptions& o) override;

  /** Execute/run the plugin */
  virtual void
  execute(RAPIOProgram * caller) override;

  /** Get our volume */
  std::shared_ptr<Volume>
  getNewVolume(const std::string & historyKey);

protected:

  /** The value of the command line argument */
  std::string myVolumeAlg;
};
}
