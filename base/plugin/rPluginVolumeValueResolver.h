#pragma once

#include "rRAPIOPlugin.h"

#include <rVolumeValueResolver.h>

namespace rapio {
class RAPIOProgram;
class RAPIOOptions;

/** Wrap the VolumeValueResolver in the newer generic plugin model.
 * Possibly we could cleanup/refactor a bit...for now we wrap the
 * global VolumeValueResolver class. */
class PluginVolumeValueResolver : public RAPIOPlugin {
public:

  /** Create a VolumeValueResolver plugin */
  PluginVolumeValueResolver(const std::string& name) : RAPIOPlugin(name){ }

  /** Declare plugin. */
  static bool
  declare(RAPIOProgram * owner, const std::string& name = "resolver");

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

  /** Return the resolver we have.  Use getPlugin(name) to access this. */
  std::shared_ptr<VolumeValueResolver>
  getVolumeValueResolver();

protected:
  /** The value of the command line argument */
  std::string myResolverAlg;

  /** The resolver we represent */
  std::shared_ptr<VolumeValueResolver> myResolver;
};
}
