#pragma once

#include "rRAPIOPlugin.h"

namespace rapio {
class RAPIOProgram;
class RAPIOOptions;

/** Subclass of plugin adding realtime mode ability (-r) */
class PluginRealTime : public RAPIOPlugin
{
public:

  /** Create a record filter plugin */
  PluginRealTime(const std::string& name) : RAPIOPlugin(name){ }

  /** Declare plugin. */
  static bool
  declare(RAPIOProgram * owner, const std::string& name = "r");

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

  /** Are we in one of the daemon (non-stopping) modes? */
  bool
  isDaemon();

  /** Are we in one of the archive (process all) modes? */
  bool
  isArchive();

  /** Our read mode for how records should be handled */
  std::string myReadMode;
};
}
