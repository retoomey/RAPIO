#pragma once

#include <rRecordNotifier.h>
#include <rVolumeValueResolver.h>
#include <rTerrainBlockage.h>
#include <rElevationVolume.h>
#include <rLLCoverageArea.h>

namespace rapio {
class RAPIOProgram;
class RAPIOOptions;

/** A program plugin interface.
 * The design idea here is to encapsulate abilities like heartbeat, etc. within
 * its own module along with its related member fields.
 *
 * Plugins currently call back to the program using virtual plug methods in RAPIOProgram
 *
 * @author Robert Toomey
 */
class RAPIOPlugin : public Utility {
public:

  /** Declare plugin with unique name */
  RAPIOPlugin(const std::string& name) : myName(name), myActive(false){ }

  /** Get name of plugin */
  std::string
  getName()
  {
    return myName;
  }

  /** Return if plugin is active */
  virtual bool
  isActive()
  {
    return myActive;
  }

  /** Declare options used */
  virtual void
  declareOptions(RAPIOOptions& o)
  { }

  /** Declare advanced help for the plugin */
  virtual void
  addPostLoadedHelp(RAPIOOptions& o)
  { }

  /** Process our options */
  virtual void
  processOptions(RAPIOOptions& o)
  { }

  /** Setup/run the plugin */
  virtual void
  execute(RAPIOProgram * caller)
  { }

protected:
  /** Name of plug, typically related to command line such as 'sync', 'web' */
  std::string myName;

  /** Simple flag telling if plugin is active/running, etc. */
  bool myActive;
};

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

protected:

  /** The cronlist for heartbeat/sync if any */
  std::string myCronList;
};

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

/** Subclass of plugin adding output filter ability (-O) */
class PluginProductOutputFilter : public RAPIOPlugin
{
public:

  /** Create a record filter plugin */
  PluginProductOutputFilter(const std::string& name) : RAPIOPlugin(name){ }

  /** Declare plugin. */
  static bool
  declare(RAPIOProgram * owner, const std::string& name = "O");

  /** Declare options for the plugin */
  virtual void
  declareOptions(RAPIOOptions& o) override;

  /** Declare advanced help for the plugin */
  virtual void
  addPostLoadedHelp(RAPIOOptions& o) override;

  /** Process our options */
  virtual void
  processOptions(RAPIOOptions& o) override;

  /** Add a static key for the -O help.  Note that keys can be static, such as
   * '2D' to refer to a class of product, or currently you can also use the
   * DataType typename as a dynamic key. */
  virtual void
  declareProduct(const std::string& key, const std::string& help)
  {
    myKeys.push_back(key);
    myKeyHelp.push_back(help);
  }

  /** Is product with this key wanted? */
  virtual bool
  isProductWanted(const std::string& key);

  /** Resolve product name from the -O Key=Resolved pairs.*/
  virtual std::string
  resolveProductName(const std::string& key, const std::string& defaultName);

protected:

  /** List of keys for -O help */
  std::vector<std::string> myKeys;

  /** List of help for -O help */
  std::vector<std::string> myKeyHelp;
};

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

/** Wrap the Volume in the newer generic plugin model.
 * Possibly we could cleanup/refactor a bit...for now we wrap the
 * global Volume class. */
class PluginVolume : public RAPIOPlugin {
public:

  /** Create a Volume plugin */
  PluginVolume(const std::string& name) : RAPIOPlugin(name){ }

  /** Declare plugin. */
  static bool
  declare(RAPIOProgram * owner, const std::string& name = "volume");

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

  /** Get our volume */
  std::shared_ptr<Volume>
  getNewVolume(const std::string & historyKey);

protected:

  /** The value of the command line argument */
  std::string myVolumeAlg;
};

enum class PartitionType { none, tile, tree };

/** Store partition information for breaking up a grid
 * into subpieces */
class PartitionInfo : public Data {
public:
  PartitionType myParamType;                ///< Type of partition such as none, tile, tree
  std::string myParamValue;                 ///< Param such as 'tile:2x2:1'
  std::vector<size_t> myDims;               ///< Dimensions of the partitioning
  std::vector<LLCoverageArea> myPartitions; ///< Partitions of the global grid
  size_t myPartitionNumber;                 ///< Selected partition

  /** Log the partition information */
  void
  printTable();
};

/** Partition option for partitioning a grid */
class PluginPartition : public RAPIOPlugin {
public:

  /** Create a Partition plugin */
  PluginPartition(const std::string& name) : RAPIOPlugin(name), myPartitionNumber(0), myValid(false){ }

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

  /** Get the partition type */
  std::string
  getPartitionType()
  {
    return myPartitionType;
  }

  /** Get the partition number we use (numbered from 1) */
  size_t
  getPartitionNumber()
  {
    return myPartitionNumber;
  }

  /** Get the dimensions of the partition type */
  std::vector<size_t>
  getDimensions()
  {
    return myDims;
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

  /** The type of partitioning, if any (lowercase). */
  std::string myPartitionType;

  /** The dimensions of partitioning, if any */
  std::vector<size_t> myDims;

  /** The partition number.  Zero means none.  Tiles are numbered
   * left to right, top to bottom, starting with 1 */
  size_t myPartitionNumber;

  /** Did we parse correctly? */
  bool myValid;
};
}
