#include "rPartitionInfo.h"

using namespace rapio;
using namespace std;

void
PartitionInfo::printTable() const
{
  LogInfo("Partitioning method: '" << myParamValue << "'.\n");
  if (!(myParamType == PartitionType::none)) {
    // Print out the partition information and mark the active one, if any
    // FIXME: This assumes 2 dimensions, if we ever do heights we'll have
    // to modify this code.
    auto& dims = myDims;
    size_t use = myPartitionNumber;
    size_t at  = 0;
    for (size_t dy = 0; dy < dims[1]; ++dy) {
      for (size_t dx = 0; dx < dims[0]; ++dx) {
        const char marker = (at + 1 == use) ? '*' : ' ';
        LogInfo("   " << marker << at + 1 << " " << myPartitions[at] << "\n");
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
    LogInfo("    X BOUNDARY:" << s.str() << "\n");

    std::stringstream t;
    for (size_t y = 0; y < myPartBoundaryY.size(); ++y) {
      auto& v = myPartBoundaryY[y];
      t << v;
      if (y + 1 < myPartBoundaryY.size()) { t << ", "; }
    }
    LogInfo("    Y BOUNDARY:" << t.str() << "\n");
  }
} // PartitionInfo::printTable
