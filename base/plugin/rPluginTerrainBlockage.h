#pragma once

#include "rRAPIOPlugin.h"
#include <rTerrainBlockage.h>

namespace rapio {
class RAPIOProgram;
class RAPIOOptions;

/** Wrap the TerrainBlockage in the newer generic plugin model.
 * Possibly we could cleanup/refactor a bit...for now we wrap the
 * global TerrainBlockage class. */
class PluginTerrainBlockage : public RAPIOPlugin {
public:

  /** Create a TerrainBlockage plugin */
  PluginTerrainBlockage(const std::string& name) : RAPIOPlugin(name){ }

  /** Declare plugin. */
  static bool
  declare(RAPIOProgram * owner, const std::string& name = "terrain");

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

  /** Attempt to create a new TerrainBlockage from our param */
  std::shared_ptr<TerrainBlockage>
  getNewTerrainBlockage(
    const LLH         & radarLocation,
    const LengthKMs   & radarRangeKMs, // Range after this is zero blockage
    const std::string & radarName,
    LengthKMs         minTerrainKMs = 0,    // Bottom beam touches this we're blocked
    AngleDegs         minAngle      = 0.1); // Below this, no blockage occurs

protected:

  /** The value of the command line argument */
  std::string myTerrainAlg;
};
}
