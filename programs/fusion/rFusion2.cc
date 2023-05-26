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
  Stage2Data::receive(d); // Hide internal format in the stage2 data
} // RAPIOFusionTwoAlg::processNewData

int
main(int argc, char * argv[])
{
  RAPIOFusionTwoAlg alg = RAPIOFusionTwoAlg();

  alg.executeFromArgs(argc, argv);
} // main
