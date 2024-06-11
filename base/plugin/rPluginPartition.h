#pragma once

#include "rRAPIOPlugin.h"

#include <rLLCoverageArea.h>
#include <rPartitionInfo.h>

namespace rapio {
class RAPIOProgram;
class RAPIOOptions;

/** Partition option for partitioning a grid */
class PluginPartition : public RAPIOPlugin {
public:

  /** Create a Partition plugin */
  PluginPartition(const std::string& name) : RAPIOPlugin(name), myValid(false){ }

  /** Declare plugin. */
  static bool
  declare(RAPIOProgram * owner, const std::string& name = "partition");

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

  /** The value of the command line argument */
  std::string
  getParamValue()
  {
    return myPartitionAlg;
  }

  /** Get if we parsed correctly.  Checked by algorithm */
  bool
  isValid()
  {
    return myValid;
  }

  /** Get the partition info, if able for this partition */
  bool
  getPartitionInfo(const LLCoverageArea& grid, PartitionInfo& info);

protected:

  /** The value of the command line argument */
  std::string myPartitionAlg;

  /** The partition info we hold (without subgrid calculations)*/
  PartitionInfo myPartitionInfo;

  /** Did we parse correctly? */
  bool myValid;
};
}
