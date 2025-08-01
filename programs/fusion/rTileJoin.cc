#include "rTileJoin.h"

using namespace rapio;

std::string
TileJoinDatabase::getKey(std::shared_ptr<LatLonGrid> l)
{
  // Create a 'key' for this incoming LatLonGrid
  const std::string aTypeName = l->getTypeName();
  const std::string aSubType  = l->getSubType();
  // Adding time as key.  We're assuming each partition has a matching unique time.
  // FIXME: This might not be true of post MRMS algs.  Should be for fusion though
  Time dataTime         = l->getTime();
  const std::string key = aTypeName + aSubType + dataTime.getString();

  return key;
}

bool
TileJoinDatabase::add(std::shared_ptr<LatLonGrid> l, std::string& databaseKey)
{
  // Create a 'key' for this incoming LatLonGrid
  const std::string key = getKey(l);
  const Time dataTime   = l->getTime();

  databaseKey = key;

  // Find partition of the LatLonGrid
  const int partNumber = getPartitionNumber(l);

  // Forced test: static int partNumber = -1;
  // partNumber++;

  // Check partition number
  if (partNumber < 0) {
    LogSevere("Couldn't find partition number for '" << key << "', ignoring. Maybe this data isn't in the grid?\n");
    return false;
  }
  LogInfo("LatLonGrid '" << key << "' has partition " << partNumber << "\n");

  // Look up the entries for this key
  auto it = myEntries.find(key);

  if (it == myEntries.end()) {
    LogInfo("Creating tile cache for '" << key << "'\n");
    // myEntries[key] = TileJoinDatabaseEntry(myPartitionInfo.size(), dataTime);
    myEntries.emplace(key, TileJoinDatabaseEntry(myPartitionInfo.size(), dataTime));
  }

  // Add this item.
  bool full = myEntries[key].add(l, partNumber);

  if (full) {
    return true;
  }
  return false;
} // TileJoinDatabase::add

std::vector<std::string>
TileJoinDatabase::getExpiredKeys(const Time& t)
{
  std::vector<std::string> expiredList;

  for (const auto& p:myEntries) {
    if (p.second.getTime() < t) {
      LogInfo("key '" << p.first << "' with time " << p.second.getTime().getString() << " has expired.\n");
      expiredList.push_back(p.first);
    }
  }
  return expiredList;
}

void
TileJoinDatabase::finalizeEntry(const std::string& databaseKey, std::shared_ptr<LatLonGrid>& out)
{
  LogInfo("Generating a final grid for key '" << databaseKey << "'\n");
  auto it = myEntries.find(databaseKey);

  if (myArrayAlgorithm == nullptr) {
    myArrayAlgorithm = std::make_shared<NearestNeighbor>();
  }

  if (it == myEntries.end()) {
    LogInfo("Key missing?  Can't generate final grid '" << databaseKey << "'\n");
    return;
  }

  auto &grids = myEntries[databaseKey].myLatLonGrids;

  bool first = true;

  for (auto p:grids) {
    if (p != nullptr) { // Missing a tile which can happen
      // Set time, etc. to the first LatLonGrid.
      if (first) {
        out->setTime(p->getTime());
        out->setUnits(p->getUnits());
        out->setTypeName(p->getTypeName());
        out->setSubType(p->getSubType());
        // Sync the height to the first tile
        LLH f = p->getLocation();
        LLH l = out->getLocation();
        l.setHeightKM(f.getHeightKM());
        out->setLocation(l);
        auto w = out->getFloat2D();
        w->fill(Constants::DataUnavailable);
        first = false;
      }

      // Insert p into the final grid. Note, the tiles 'should' all
      // be exact, at least for moment.  By remapping we do extra
      // work but then we don't have to worry about exact coordinates
      p->RemapInto(out, myArrayAlgorithm);
    }
  }

  // Finalize means remove the key, we're done.
  myEntries.erase(databaseKey);
} // TileJoinDatabase::finalizeEntry

void
TileJoinDatabase::printInfo() const
{
  LogInfo("----------------Entries----------------\n");
  for (const auto& p:myEntries) {
    LogInfo("For key '" << p.first << "' with time " << p.second.getTime().getString() << "\n");
    p.second.printInfo();
  }
}

void
TileJoinAlg::declarePlugins()
{
  // Partitioner
  PluginPartition::declare(this, "partition");
}

void
TileJoinAlg::declareOptions(RAPIOOptions& o)
{
  o.setDescription(
    "RAPIO Untiler Algorithm.  Combined data from partitions generated by rFusion2.");
  o.setAuthors("Robert Toomey");

  // legacy use the t, b and s to define a grid
  o.declareLegacyGrid();

  // Default sync heartbeat to 30 seconds, format is seconds then mins
  o.setDefaultValue("sync", "*/30 * * * * *");
} // TileJoinAlg::declareOptions

/** RAPIOAlgorithms process options on start up */
void
TileJoinAlg::processOptions(RAPIOOptions& o)
{
  // ----------------------------------------
  // Check partition information
  bool success = false;
  auto part    = getPlugin<PluginPartition>("partition");

  if (part) {
    o.getLegacyGrid(myFullGrid);
    success = part->getPartitionInfo(myFullGrid, myPartitionInfo);
  }
  if (!success) {
    LogSevere("Failed to load and/or parse partition information!\n");
    exit(1);
  }
  // myPartitionInfo.printTable();
} // TileJoinAlg::processOptions

int
TileJoinDatabase::getPartitionNumber(std::shared_ptr<LatLonGrid>& l)
{
  // We'll check the center of the LatLonGrid and find the partition from that
  // it should be 'good enough' for moment.
  // FIXME: Check all coordinates for exact match/size, etc.  Validate better
  // Right now you could mess up data in and break things.
  int partNumber = myPartitionInfo.getPartitionNumber(l->getCenterLocation());

  // LogSevere("INCOMING CENTER:" << l->getCenterLocation() << ", partition is " << partNumber << "\n");
  return partNumber;
}

void
TileJoinAlg::firstSetup()
{
  static bool first = true;

  // Initial setup of database, etc.
  if (first) {
    myTileJoinDatabase = std::make_shared<TileJoinDatabase>(myPartitionInfo);

    // Make a LatLonGrid we'll use to store the output tiles into.  Name and
    // units, etc. will come from the tiles.
    myLLGOutput = LatLonGrid::Create("Cache", "dBZ", Time(), myFullGrid);
    // myLLGOutput->fillPrimary(Constants::DataUnavailable);
  }
  first = false;
}

void
TileJoinAlg::processNewData(rapio::RAPIOData& d)
{
  // Look for any data the system knows how to read...
  auto l = d.datatype<rapio::LatLonGrid>();

  if (l != nullptr) {
    // Make sure database, etc. ready to go
    firstSetup();

    // We can merge multiple 2D layers/grids by time.  So we need to store tiles until
    // one of two conditions:
    // 1.  We have all the tiles for a time.  Then we write them.
    // 2.  We time out on that tile array and give up waiting and save what we have

    // Create a 'key' for this incoming LatLonGrid
    const std::string aTypeName = l->getTypeName();
    const std::string aSubType  = l->getSubType();
    const std::string key       = aTypeName + aSubType;

    std::string databaseKey;

    /*
     * // Forced test case...
     * std::string databaseKey;
     * myTileJoinDatabase->add(l, databaseKey);
     * LogSevere("Doing second add to check..\n");
     * myTileJoinDatabase->add(l, databaseKey);
     * LogSevere("Doing third add to check..\n");
     * myTileJoinDatabase->add(l, databaseKey);
     * LogSevere("Doing fourth add to check..\n");
     */

    // if full on add, we'll write it out immediately
    // FIXME: One issue with this current design is if an old grid is missing a tile, and the
    // history is long, it's possible that the old one will write out (expire) after the new one, which we
    // probably don't want.  When we write out a 'full' one for a time, we should first write out all the
    // partial old ones for the same typename.  This would require more database work to store times
    // separate from keys.  Baby steps
    if (myTileJoinDatabase->add(l, databaseKey)) {
      myTileJoinDatabase->finalizeEntry(databaseKey, myLLGOutput);
      std::map<std::string, std::string> myOverrides; // None for now
      writeOutputProduct(myLLGOutput->getTypeName(), myLLGOutput, myOverrides);
    }

    // myTileJoinDatabase->printInfo();
  }
} // TileJoinAlg::processNewData

void
TileJoinAlg::processHeartbeat(const Time& n, const Time& p)
{
  // Make sure database, etc. ready to go
  firstSetup();

  // Expire tile groups that have waited long enough.
  const Time cutoffTime = p - myMaximumHistory;
  auto list = myTileJoinDatabase->getExpiredKeys(cutoffTime);

  if (list.size() > 0) {
    LogInfo("We have some expired groups...writing them.\n");
  }
  for (auto databaseKey:list) {
    myTileJoinDatabase->finalizeEntry(databaseKey, myLLGOutput);
    std::map<std::string, std::string> myOverrides; // None for now
    writeOutputProduct(myLLGOutput->getTypeName(), myLLGOutput, myOverrides);
  }
}

int
main(int argc, char * argv[])
{
  // Create and run fusion stage 1
  TileJoinAlg alg = TileJoinAlg();

  alg.executeFromArgs(argc, argv);
}
