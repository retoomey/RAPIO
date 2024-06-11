#pragma once

#include <rRAPIOPlugin.h>

#include <rRecordNotifier.h>

namespace rapio {
class RAPIOProgram;
class RAPIOOptions;

/** Subclass of plugin adding notification ability (-n)*/
class PluginNotifier : public RAPIOPlugin
{
public:

  /** Create a notifier plugin */
  PluginNotifier(const std::string& name) : RAPIOPlugin(name){ }

  /** Declare plugin. */
  static bool
  declare(RAPIOProgram * owner, const std::string& name = "n");

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

public:

  /** Global notifiers we are sending notification of new records to */
  static std::vector<std::shared_ptr<RecordNotifierType> > theNotifiers;

protected:

  /** The mode of web server, if any */
  std::string myNotifierList;
};
}
