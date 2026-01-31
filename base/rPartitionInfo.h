#pragma once

#include <rLLCoverageArea.h>
#include <rStrings.h>
#include <rError.h>
#include <rLL.h>

#include <string>
#include <vector>

namespace rapio {
/** Store partition information for breaking up a grid
 * into a 2D grid of subgrids.
 *
 * @author Robert Toomey
 * @ingroup rapio_data
 * @brief Stores partitions of a grid.
 */
class PartitionInfo {
public:

  /** Type of partition */
  enum class Type { none, tile, tree };

  /** Create a partition info */
  PartitionInfo() : myPartitionNumber(0){ }

  /** Set the partition value string */
  inline void
  setParamValue(const std::string& s)
  {
    myParamValue = s;
  }

  /** Get the partition value string */
  inline std::string
  getParamValue() const
  {
    return myParamValue;
  }

  /** Set partition type from a string */
  bool
  setPartitionType(const std::string& partTypeIn);

  /** Get the partition type */
  inline Type
  getPartitionType() const
  {
    return myParamType;
  }

  /** Get selection partition number */
  inline size_t
  getSelectedPartitionNumber() const
  {
    return myPartitionNumber;
  }

  /** Set the selected partition number */
  inline void
  setSelectedPartitionNumber(size_t p)
  {
    myPartitionNumber = p;
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
  bool
  partition(const LLCoverageArea& grid);

  /** Get a reference to the selected partition, only good if valid */
  inline const LLCoverageArea&
  getSelectedPartition() const
  {
    return myPartitions[myPartitionNumber - 1];
  }

  /** Get the number of partitions we store */
  inline size_t
  size() const
  {
    return myPartitions.size();
  }

  // The nextX, nextY and index are called in the fusion loop,
  // so should be inline

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

  /** Get index into partition list from 2D index partition dimensions */
  inline size_t
  index(size_t x, size_t y) const
  {
    size_t width = myDims[0];

    return y * width + x;
  }

  // ^^ Called by fusion loop

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

  // Called by rTile to join partition grids

  /** Get partition number, if any for given location */
  int
  getPartitionNumber(const LL& at) const;

  /** Log the partition information */
  void
  printTable() const;

protected:
  Type myParamType;           ///< Type of partition such as none, tile, tree
  std::string myParamValue;   ///< Param such as 'tile:2x2:1'
  std::vector<size_t> myDims; ///< Dimensions of the partitioning (2 currently)
  size_t myPartitionNumber;   ///< Selected partition

  // Calculated
  std::vector<LLCoverageArea> myPartitions; ///< Partitions of the global grid
  std::vector<size_t> myPartBoundaryX;      ///< Partition global boundary in X direction
  std::vector<size_t> myPartBoundaryY;      ///< Partition global boundary in Y direction
};
}
