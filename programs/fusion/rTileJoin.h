#pragma once

/** RAPIO API */
#include <RAPIO.h>
#include "rLLCoverageArea.h"

namespace rapio {
/** Database to store incoming tile data/information, from a given key to a partition list of data */
class TileJoinDatabaseEntry {
public:
  /** STL */
  TileJoinDatabaseEntry(){ }

  /** Create an entry */
  TileJoinDatabaseEntry(int partitionSize, Time t)
  {
    myLatLonGrids.resize(partitionSize);
    myTime = t;
  }

  /** Are we full...do we have a LatLonGrid for each partition location? */
  bool
  full()
  {
    // Check if we're full
    for (auto p:myLatLonGrids) {
      if (p == nullptr) { return false; }
    }
    return true;
  }

  /** Add a given LatLonGrid, return if we're full */
  bool
  add(std::shared_ptr<LatLonGrid> l, size_t partNumber)
  {
    // Double up probably a bug...
    const bool exists = (myLatLonGrids[partNumber] != nullptr);

    if (exists) {
      fLogSevere("We replaced a LatLonGrid which was unexpected?");
    }
    myLatLonGrids[partNumber] = l;

    return (full());
  }

  /** Get our time */
  Time
  getTime() const
  {
    return myTime;
  }

  /** Test print  */
  void
  printInfo() const
  {
    size_t i = 0;

    for (auto p:myLatLonGrids) {
      fLogInfo("{} {}", i, (void *) (p.get()));
      i++;
    }
  }

  /** Store up to partition size of LatLonGrids */
  std::vector<std::shared_ptr<LatLonGrid> > myLatLonGrids;

  /** Store time we reference */
  Time myTime;
};

/** Database to store incoming tile data/information */
class TileJoinDatabase {
public:

  /** Create a database for storing partitions */
  TileJoinDatabase(PartitionInfo& p) : myPartitionInfo(p){ }

  /** Get partition number for a LatLonGrid */
  int
  getPartitionNumber(std::shared_ptr<LatLonGrid>& l);

  /** Generate a database key for a given LatLonGrid */
  std::string
  getKey(std::shared_ptr<LatLonGrid> l);

  /** Add a LatLonGrid to our database, return true if full */
  bool
  add(std::shared_ptr<LatLonGrid> l, std::string& databaseKey);

  /** Get a list of expired keys given a time */
  std::vector<std::string>
  getExpiredKeys(const Time& t);

  /** Write given entry key to output and drop the entry */
  void
  finalizeEntry(const std::string& databaseKey, std::shared_ptr<LatLonGrid>& out);

  /** A Debg dump of the database structure */
  void
  printInfo() const;

protected:

  /** The partition info we're using */
  PartitionInfo myPartitionInfo;

  /** Map of keys to data */
  std::map<std::string, TileJoinDatabaseEntry> myEntries;

  /** The remapper we use to copy tiles */
  std::shared_ptr<ArrayAlgorithm> myArrayAlgorithm;
};

/** An algorithm to untile partitions created by rFusion2
 *
 * @author Robert Toomey
 */
class TileJoinAlg : public rapio::RAPIOAlgorithm {
public:

  /** Create tile algorithm */
  TileJoinAlg(){ };

  /** Declare all algorithm command line plugins */
  virtual void
  declarePlugins() override;

  /** Declare all algorithm options */
  virtual void
  declareOptions(rapio::RAPIOOptions& o) override;

  /** Process all algorithm options */
  virtual void
  processOptions(rapio::RAPIOOptions& o) override;

  /** Process a new record/datatype */
  virtual void
  processNewData(rapio::RAPIOData& d) override;

  /** Process heartbeat in subclasses.
   * @param n The actual now time triggering the event.
   * @param p The pinned sync time we're firing for. */
  virtual void
  processHeartbeat(const Time& n, const Time& p) override;

  /** First time setup of database, etc. */
  void
  firstSetup();

protected:

  /** Coordinates for the total merger grid */
  LLCoverageArea myFullGrid;

  /** Cached reusable full output grid */
  std::shared_ptr<LatLonGrid> myLLGOutput;

  /** The partition info we're using */
  PartitionInfo myPartitionInfo;

  /** The Database of tiles we hold onto */
  std::shared_ptr<TileJoinDatabase> myTileJoinDatabase;
};
}
