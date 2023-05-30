#include "rFusion2.h"
#include "rStage2Data.h"

#include <rXYZ.h>
#include <rBitset.h>
#include <rSparseVector.h>

using namespace rapio;

/** Merging requires some basic abilities.  This less data we
 * store per node the better due to the massive size
 * 1.  Need to find nodes with times matching for all XYZ
 *     This is for expiring old data at will.
 *     Current: O(N^3) search xyzStorage, delete nodes
 * 2.  Need to find nodes with radar/elevation matching
 *     Do we need elevation angle to actually merge?  It's a lot
 *     of extra data.
 * 3.  Need to iterate fast in x,y,z to get all nodes matching
 *     This is solved by xyz array with N nodes per location.
 */

void
RAPIOFusionTwoAlg::declareOptions(RAPIOOptions& o)
{
  o.setDescription(
    "RAPIO Fusion Stage Two Algorithm.  Gathers data from multiple radars for mergin values");
  o.setAuthors("Robert Toomey");

  // legacy use the t, b and s to define a grid
  o.declareLegacyGrid();
}

/** RAPIOAlgorithms process options on start up */
void
RAPIOFusionTwoAlg::processOptions(RAPIOOptions& o)
{
  o.getLegacyGrid(myFullGrid);
} // RAPIOFusionTwoAlg::processOptions

void
RAPIOFusionTwoAlg::processNewData(rapio::RAPIOData& d)
{
  auto data = Stage2Data::receive(d); // Hide internal format in the stage2 data

  LogInfo("Fusion2 Got data, counting for now:\n");

  size_t counter        = 0;
  size_t missingcounter = 0;
  size_t points         = 0;
  size_t total = 0;

  if (data) {
    float v;
    float w;
    short x;
    short y;
    short z;
    while (data->get(v, w, x, y, z)) { // add time, other stuff
      // TODO: point cloud fun stuff....
      if (counter++ < 10) {
        LogInfo("RECEIVE: " << v << ", " << w << ", (" << x << "," << y << "," << z << "\n");
      }
      if (v == Constants::MissingData) {
        missingcounter++;
      } else {
        points++;
      }
      total++;
    }
    LogInfo("Final size received: " << points << " points, " << missingcounter << " missing.  Total: " << total <<
      "\n");
  }
} // RAPIOFusionTwoAlg::processNewData

int
main(int argc, char * argv[])
{
  RAPIOFusionTwoAlg alg = RAPIOFusionTwoAlg();

  alg.executeFromArgs(argc, argv);
} // main
