// Add this at top for any BOOST test
#include "rBOOSTTest.h"

#include "rPartitionInfo.h"

using namespace rapio;

BOOST_AUTO_TEST_SUITE(GRID)

BOOST_AUTO_TEST_CASE(GRID_PARTITION)
{
  // Testing making grid
  LLCoverageArea aGrid;
  std::string topcorner = "";
  std::string botcorner = "";
  std::string spacing   = "";
  std::string gridstr   = "nw(37, -100) se(30.5, -93) h(0.5,20,NMQWD) s(0.01, 0.01)";
  bool successParseGrid = aGrid.parse(gridstr, topcorner, botcorner, spacing);

  BOOST_REQUIRE(successParseGrid == true);

  // Now test a few partitions of this grid
  PartitionInfo info;

  // size_t partX = 700; // Should be 1 to 1
  // size_t partX = 350; // Should be 001122 X
  size_t partX = 700; // Should be 001122 X
  size_t partY = 2;

  // Test partitioning a 2x2
  info.setPartitionType("tile"); // Humm could emum right?
  info.set2DDimensions(partX, partY);

  bool successPartitioning = info.partition(aGrid);

  BOOST_REQUIRE(successPartitioning == true);

  info.printTable();

  size_t aSize = info.size();

  BOOST_REQUIRE_EQUAL(aSize, partX * partY);
  // if (!successPartitioning){ return; } // Other tests will fail for sure.

  // March over a grid, checking partition back for correct range..
  size_t maxX = aGrid.getNumX();
  size_t maxY = aGrid.getNumY();

  // Partitioning vectors...tracking which partition we are in
  // Get our starting base partition indexes in X,Y
  const size_t sX = aGrid.getStartX(); // 0 in this case (if subgrid, the first cellX)
  const size_t sY = aGrid.getStartY(); // 0 in this case (if subgrid, the first cellY)
  const size_t startPartXDim = info.getXDimFor(sX);
  const size_t startPartYDim = info.getYDimFor(sY);

  // std::cout << "STARTS " << sX << ", " << sY << ", " << startPartXDim << ", " << startPartYDim << "\n";

  // Starting partition dimensions.  Say 2x10 then 0-1 and 0-9
  bool DimCheckSuccess = true;

  if ((startPartXDim > maxX) || (startPartYDim > maxY)) {
    DimCheckSuccess = false;
  }
  BOOST_REQUIRE(DimCheckSuccess == true);

  bool inPartitionCheck = true;

  size_t yPartDim = startPartYDim;

  for (size_t y = 0; y < maxY; y++) {
    info.nextY(sY + y, yPartDim); // increment dimension Y if needed
    // std::cout << yPartDim << ":";
    size_t xPartDim = startPartXDim;
    for (size_t x = 0; x < maxX; x++) {
      info.nextX(sX + x, xPartDim); // increment dimension X if needed
      // if (y == 0){
      //  std::cout << xPartDim<<":";
      // }
      size_t partIndex = info.index(xPartDim, yPartDim);
      if (partIndex > partX * partY) {
        inPartitionCheck = false;
        goto doneLoop;
      }
    }
    // if (y == 0){
    //	      std::cout << "\n";
    //      }
  }
  // std::cout << "\n";
doneLoop:
  BOOST_CHECK(inPartitionCheck == true);
}

BOOST_AUTO_TEST_SUITE_END();
