#include "rPluginPartition.h"

#include <rRAPIOAlgorithm.h>

using namespace rapio;
using namespace std;

bool
PluginPartition::declare(RAPIOProgram * owner, const std::string& name)
{
  owner->addPlugin(new PluginPartition(name));
  return true;
}

void
PluginPartition::declareOptions(RAPIOOptions& o)
{
  o.optional(myName, "none",
    "Partition algorithm, such as 'tile', or 'tree'.");
  o.addGroup(myName, "SPACE");
  // Volume::introduceSuboptions(myName, o);
}

void
PluginPartition::addPostLoadedHelp(RAPIOOptions& o)
{
  std::string help;

  help += "Partitioning allows two methods, by tile or by tree.\n";
  help += "Each method breaks its output into multiple subfolders, where each is labeled 'partitionX'.\n";
  help += "Tile: Stage1 files are split on tile boundaries, one radar can write N files.\n";
  help += "     A Stage2 can then point to each partition (This can cause time discontinuities on tile borders).\n";
  help += "Tree: Stage1 files write entirely into the partition based on radar center.\n";
  help += "     A Stage2 can read a partition containing all data for radars within it.\n";
  help +=
    "Example: partition='Tile:3x2' The grid is divided into 6 cells.  Three across (longitude) and two down (latitude).\n";
  help += "         They are numbered 123 and 456, creating 'partition1' to 'partition6'.\n";
  help += "Example: partition='Tile:3x2:2' Your alg handles tile '2' of the 6, so the top center tile.\n";

  o.addAdvancedHelp(myName, help);
}

void
PluginPartition::processOptions(RAPIOOptions& o)
{
  myPartitionAlg = o.getString(myName);
  myPartitionInfo.myParamValue = myPartitionAlg;
  size_t totalPartitions = 0;

  // ----------------------------------------------------
  // Split on tile:2x2:3 values...
  std::vector<std::string> pieces;

  Strings::splitWithoutEnds(myPartitionAlg, ':', &pieces);
  if (pieces.size() > 0) {
    // Set partition type first
    if (!myPartitionInfo.setPartitionType(pieces[0])) {
      return;
    }

    // If none, use the global grid passed in as a single partition...
    if (myPartitionInfo.myParamType == PartitionType::none) {
      myPartitionInfo.set2DDimensions(1, 1);
      myPartitionInfo.myPartitionNumber = 1;
      myValid = true;

      // ...otherwise we parse extra params.
    } else {
      // ----------------------------------------------------
      // Handle the 3x3 format
      if (pieces.size() > 1) {
        std::vector<std::string> dims;
        Strings::splitWithoutEnds(pieces[1], 'x', &dims);
        // Only allow 2 dimensions for now...
        if (dims.size() == 2) {
          try{
            size_t d1 = std::stoi(dims[0]);
            size_t d2 = std::stoi(dims[1]);
            myPartitionInfo.set2DDimensions(d1, d2);
            totalPartitions = d1 * d2;
          }catch (const std::exception& e) {
            LogSevere("Couldn't parse " << myName << " dimensions: " << pieces[1] << "\n");
          }
        } else {
          LogSevere("Couldn't parse " << myName << " dimensions: " << pieces[1] << "\n");
          return;
        }
      }

      // ----------------------------------------------------
      // Handle the tile number format (optional or -1)
      if (pieces.size() > 2) {
        try{
          int partNumber = std::stoi(pieces[2]);
          if (partNumber > totalPartitions) {
            LogSevere(
              "Partition number for '" << myPartitionAlg << "' given '" << partNumber << "' is larger than total partitions '" << totalPartitions <<
                "'\n");
            return;
          }
          myPartitionInfo.myPartitionNumber = partNumber;
        }catch (const std::exception& e) {
          LogSevere("Couldn't parse " << myName << " partition number: " << pieces[2] << "\n");
          return;
        }
      }
    }
  } else {
    LogSevere("Couldn't parse " << myName << "\n");
  }
  myValid = true;
} // PluginPartition::processOptions

bool
PluginPartition::getPartitionInfo(
  const LLCoverageArea & grid,
  PartitionInfo        & info)
{
  if (isValid()) {
    info = myPartitionInfo; // copy
    info.partition(grid);
    info.printTable();
  } else {
    LogSevere("Partition info incorrect.\n");
    return false;
  }
  return true;
} // PluginPartition::getPartitionInfo

void
PluginPartition::execute(RAPIOProgram * caller)
{
  // Nothing, partition information is pulled by algorithm
}
