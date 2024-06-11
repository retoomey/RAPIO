#pragma once

#include "rRAPIOPlugin.h"

namespace rapio {
class RAPIOProgram;
class RAPIOOptions;

/** Subclass of plugin adding record filter ability (-I) */
class PluginRecordFilter : public RAPIOPlugin
{
public:

  /** Create a record filter plugin */
  PluginRecordFilter(const std::string& name) : RAPIOPlugin(name){ }

  /** Declare plugin. */
  static bool
  declare(RAPIOProgram * owner, const std::string& name = "I");

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
};
}
