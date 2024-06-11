#pragma once

#include "rRAPIOPlugin.h"

#include <rIndexType.h>

namespace rapio {
class RAPIOProgram;
class RAPIOOptions;

/** Subclass of plugin adding ingest index ability (-i) */
class PluginIngestor : public RAPIOPlugin
{
public:

  /** Create an ingestor plugin */
  PluginIngestor(const std::string& name) : RAPIOPlugin(name){ }

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

protected:

  /** Indexes we are successfully attached to */
  std::vector<std::shared_ptr<IndexType> > myConnectedIndexes;
};
}
