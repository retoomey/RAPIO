#pragma once

#include <rData.h>
#include <rLLCoverageArea.h>
#include <rStrings.h>
#include <rError.h>

#include <string>
#include <vector>

namespace rapio {
enum class PartitionType { none, tile, tree };

/** Store partition information for breaking up a grid
 * into a 2D grid of subgrids.
 *
 * @author Robert Toomey
 */
class PartitionInfo : public Data {
public:
  /** Create a partition info */
  PartitionInfo() : myPartitionNumber(0){ }

  PartitionType myParamType;  ///< Type of partition such as none, tile, tree
  std::string myParamValue;   ///< Param such as 'tile:2x2:1'
  std::vector<size_t> myDims; ///< Dimensions of the partitioning (2 currently)
  size_t myPartitionNumber;   ///< Selected partition

  // Calculated
  std::vector<LLCoverageArea> myPartitions; ///< Partitions of the global grid
  std::vector<size_t> myPartBoundaryX;      ///< Partition global boundary in X direction
  std::vector<size_t> myPartBoundaryY;      ///< Partition global boundary in Y direction

  /** Get a reference to the selected partition, only good if valid */
  const LLCoverageArea&
  getSelectedPartition()
  {
    return myPartitions[myPartitionNumber - 1];
  }

  /** Set partition type from a string */
  bool
  setPartitionType(const std::string& partTypeIn)
  {
    std::string partType = Strings::makeLower(partTypeIn);

    if (!((partType == "tree") || (partType == "tile") || (partType == "none"))) {
      LogSevere("Unrecognized partition type: '" << partType << "'\n");
      return false;
    }
    if (partType == "tile") {
      myParamType = PartitionType::tile;
    } else if (partType == "tree") {
      myParamType = PartitionType::tree;
    } else {
      myParamType = PartitionType::none;
    }
    return true;
  }

  /** Set the dimensions */
  void
  set2DDimensions(size_t x, size_t y)
  {
    myDims.clear();
    myDims.push_back(x);
    myDims.push_back(y);
  }

  /** Partition a global grid using current dimensions */
  void
  partition(const LLCoverageArea& grid)
  {
    /** Our partition has all the information, except for the
     * global grid generated info, which we will fill in now.
     * The plugin set myDims for us. */

    // Break up the grid partitions, assuming 2 dimensions
    // This 'could' be 1x1 or the global as well for say 'none'
    grid.tile(myDims[0], myDims[1], myPartitions);

    // March row order, creating boundary markers in global grid
    // space for each partition.
    // Might be easier to do this in the 'tile' method
    size_t at        = 0;
    size_t xBoundary = 0;
    size_t yBoundary = 0;

    for (size_t y = 0; y < myDims[1]; ++y) {
      auto& g = myPartitions[at];
      for (size_t x = 0; x < myDims[0]; ++x) {
        // Add global boundaries in the X direction
        if (x == 0) {
          xBoundary += g.getNumX();
          myPartBoundaryX.push_back(xBoundary);
        }
      }
      // Add global boundaries in the Y direction
      yBoundary += g.getNumY();
      myPartBoundaryY.push_back(yBoundary);
      at++;
    }
  }

  /** Get index into partition list from 2D index partition dimensions */
  size_t
  index(size_t x, size_t y) const
  {
    size_t width = myDims[0];

    return y * width + x;
  }

  /** Get the number of partitions we store */
  size_t
  size() const
  {
    return myPartitions.size();
  }

  /** Increment dimension X if needed for a given global X */
  inline void
  nextX(size_t globalX, size_t& atDimX) const
  {
    if (globalX >= myPartBoundaryX[atDimX]) {
      atDimX++;
    }
  }

  /** Increment dimension Y if needed for a given global Y */
  inline void
  nextY(size_t globalY, size_t& atDimY) const
  {
    if (globalY >= myPartBoundaryY[atDimY]) {
      atDimY++;
    }
  }

  /** Get the index partition dimension in X we are in for a global grid X.
   * Note if no partitions than we return 0 (we intend this) */
  size_t
  getXDimFor(size_t globalX) const
  {
    size_t partX = 0;

    for (auto& x:myPartBoundaryX) {
      if (globalX < x) {
        return partX;
      }
      partX++;
    }
    return partX;
  }

  /** Get the index partition dimension in Y we are in for a global grid Y.
   * Note if no partitions than we return 0 (we intend this) */
  size_t
  getYDimFor(size_t globalY) const
  {
    size_t partY = 0;

    for (auto& y:myPartBoundaryY) {
      if (globalY < y) {
        return partY;
      }
      partY++;
    }
    return partY;
  }

  /** Log the partition information */
  void
  printTable() const;
};
}
