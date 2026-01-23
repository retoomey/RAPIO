#include "rPartitionInfo.h"

using namespace rapio;
using namespace std;


bool
PartitionInfo::setPartitionType(const std::string& partTypeIn)
{
  std::string partType = Strings::makeLower(partTypeIn);

  if (!((partType == "tree") || (partType == "tile") || (partType == "none"))) {
    fLogSevere("Unrecognized partition type: '{}'", partType);
    return false;
  }
  if (partType == "tile") {
    myParamType = Type::tile;
  } else if (partType == "tree") {
    myParamType = Type::tree;
  } else {
    myParamType = Type::none;
  }
  return true;
}

bool
PartitionInfo::partition(const LLCoverageArea& grid)
{
  /** Our partition has all the information, except for the
   * global grid generated info, which we will fill in now.
   * The plugin set myDims for us. */

  // Break up the grid partitions, assuming 2 dimensions
  // This 'could' be 1x1 or the global as well for say 'none'
  if (!grid.tile(myDims[0], myDims[1], myPartitions)) {
    return false;
  }
  ;

  // March row order, creating boundary markers in global grid
  // space for each partition.
  // Might be easier to do this in the 'tile' method
  size_t at        = 0;
  size_t xBoundary = 0;
  size_t yBoundary = 0;

  for (size_t y = 0; y < myDims[1]; ++y) {
    for (size_t x = 0; x < myDims[0]; ++x) {
      auto& g = myPartitions[at];

      // Note we just need one 'size' of the cube in each
      // direction, so use x=0, y=0

      // For first X, add boundaries for each Y
      if (x == 0) {
        // Add global boundaries in the Y direction
        yBoundary += g.getNumY();
        myPartBoundaryY.push_back(yBoundary);
      }

      // For first Y, add boundaries for each X
      if (y == 0) {
        xBoundary += g.getNumX();
        myPartBoundaryX.push_back(xBoundary);
      }
      at++;
    }
  }
  return true;
} // partition

/** Get partition number, if any for given location */
int
PartitionInfo::getPartitionNumber(const LL& at) const
{
  auto& lat = at.getLatitudeDeg();
  auto& lon = at.getLongitudeDeg();
  int part  = 0;

  for (auto& p:myPartitions) {
    // Inclusive on top left of partition.
    if ( (lat <= p.getNWLat()) && (lat > p.getSELat()) &&
      (lon >= p.getNWLon()) && (lon < p.getSELon()) )
    {
      return part;
    }
    part++;
  }
  return -1;
}

void
PartitionInfo::printTable() const
{
  fLogInfo("Partitioning method: '{}'.", myParamValue);
  if (!(myParamType == Type::none)) {
    // Print out the partition information and mark the active one, if any
    // FIXME: This assumes 2 dimensions, if we ever do heights we'll have
    // to modify this code.
    auto& dims = myDims;
    size_t use = myPartitionNumber;
    size_t at  = 0;
    for (size_t dy = 0; dy < dims[1]; ++dy) {
      for (size_t dx = 0; dx < dims[0]; ++dx) {
        const char marker = (at + 1 == use) ? '*' : ' ';
        fLogInfo("   {}{} {}", marker, at + 1, myPartitions[at]);
        at++;
      }
    }

    // Debugging
    std::stringstream s;
    for (size_t x = 0; x < myPartBoundaryX.size(); ++x) {
      auto& v = myPartBoundaryX[x];
      s << v;
      if (x + 1 < myPartBoundaryX.size()) { s << ", "; }
    }
    fLogInfo("    X BOUNDARY:{}", s.str());

    std::stringstream t;
    for (size_t y = 0; y < myPartBoundaryY.size(); ++y) {
      auto& v = myPartBoundaryY[y];
      t << v;
      if (y + 1 < myPartBoundaryY.size()) { t << ", "; }
    }
    fLogInfo("    Y BOUNDARY:{}", t.str());
  }
} // PartitionInfo::printTable
