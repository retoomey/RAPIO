#pragma once

#include "rRAPIOPlugin.h"
#include "rHeartbeat.h"

namespace rapio {
class RAPIOProgram;
class RAPIOOptions;

/** Subclass of plugin adding a heartbeat ability (-sync) */
class PluginHeartbeat : public RAPIOPlugin {
public:

  /** Create a heartbeat plugin */
  PluginHeartbeat(const std::string& name) : RAPIOPlugin(name){ }

  /** Declare plugin. */
  static bool
  declare(RAPIOProgram * owner, const std::string& name = "sync");

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

  /** Handle a post record event */
  virtual void
  postRecordEvent(RAPIOProgram * caller, const Record& rec) override;

protected:

  /** The cronlist for heartbeat/sync if any */
  std::string myCronList;

  /** The heartbeat we call */
  std::shared_ptr<Heartbeat> myHeartBeat;
};
}
