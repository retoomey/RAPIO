#pragma once

#include "rRAPIOPlugin.h"

namespace rapio {
class RAPIOProgram;
class RAPIOOptions;

/** Subclass of plugin adding output ability (-o) */
class PluginProductOutput : public RAPIOPlugin
{
public:

  /** Create a output plugin */
  PluginProductOutput(const std::string& name) : RAPIOPlugin(name){ }

  /** Declare plugin. */
  static bool
  declare(RAPIOProgram * owner, const std::string& name = "o");

  /** Declare options for the plugin */
  virtual void
  declareOptions(RAPIOOptions& o) override;

  /** Declare advanced help for the plugin */
  virtual void
  addPostLoadedHelp(RAPIOOptions& o) override;

  /** Process our options */
  virtual void
  processOptions(RAPIOOptions& o) override;
};
}
