#pragma once

#include "rRAPIOPlugin.h"

namespace rapio {
class RAPIOProgram;
class RAPIOOptions;

/** Subclass of plugin doing webserver ability (-web) */
class PluginWebserver : public RAPIOPlugin {
public:

  /** Create a web server plugin */
  PluginWebserver(const std::string& name) : RAPIOPlugin(name){ }

  /** Declare plugin. */
  static bool
  declare(RAPIOProgram * owner, const std::string& name = "web");

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

protected:

  /** The mode of web server, if any */
  std::string myWebServerMode;
};
}
