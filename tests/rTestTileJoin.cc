#include "rBOOSTTest.h"
#include "rLatLonGrid.h"
#include "rProcessTimer.h"
#include "rConstants.h"
#include "rError.h"
#include "rArith.h"
#include "rPartitionInfo.h"

using namespace rapio;

BOOST_AUTO_TEST_SUITE(TILE_JOIN_FOUR_WAY_TESTS)

BOOST_AUTO_TEST_CASE(TEST_FOUR_TILE_ASSEMBLY)
{
  // 1. Define the full coverage area (1000x1000 grid)
  const size_t fullLats  = 1000;
  const size_t fullLons  = 1000;
  const float latSpacing = 0.01f;
  const float lonSpacing = 0.01f;
  const float nwLat      = 50.0f;
  const float nwLon      = -100.0f;

  // Calculate SE corner
  const float seLat = nwLat - (fullLats * latSpacing);
  const float seLon = nwLon + (fullLons * lonSpacing);

  LLCoverageArea fullArea(nwLat, nwLon, seLat, seLon, latSpacing, lonSpacing, fullLons, fullLats);
  auto outGrid = LatLonGrid::Create("Final", "dBZ", Time(), fullArea);

  outGrid->getFloat2D()->fill(Constants::DataUnavailable);

  // 2. Set up PartitionInfo for a 2x2 grid
  PartitionInfo pInfo;

  pInfo.setPartitionType("tile");
  pInfo.set2DDimensions(2, 2);
  BOOST_REQUIRE(pInfo.partition(fullArea));
  BOOST_REQUIRE_EQUAL(pInfo.size(), 4);

  Time t = Time::CurrentTime();

  // We will store the 4 tiles here to simulate the TileJoinDatabase map
  std::vector<std::shared_ptr<LatLonGrid> > grids;

  // 3. Create the 4 tiles
  for (size_t i = 1; i <= 4; ++i) {
    pInfo.setSelectedPartitionNumber(i);
    LLCoverageArea tileArea = pInfo.getSelectedPartition();

    auto tile = LatLonGrid::Create("Tile", "dBZ", t, tileArea);

    // Fill each tile with a unique value: 10.0, 20.0, 30.0, 40.0
    float uniqueValue = static_cast<float>(i * 10.0f);
    tile->getFloat2D()->fill(uniqueValue);

    grids.push_back(tile);
  }

  // 4. Perform the real blit using the production codebase
  {
    ProcessTimer pt("4-Tile Assembly Blit");
    for (auto p : grids) {
      // If this fails, the test will abort and print the failure
      BOOST_REQUIRE(p->OverlayAligned(outGrid));
    }
  }

  // 5. Verify the output grid is perfectly assembled
  auto& dstData        = outGrid->getFloat2DRef();
  const size_t dstRows = outGrid->getNumLats();
  const size_t dstCols = outGrid->getNumLons();

  bool match = true;

  for (size_t y = 0; y < dstRows; ++y) {
    for (size_t x = 0; x < dstCols; ++x) {
      float expectedValue = 0.0f;

      // Determine expected value based on quadrants
      if ((y < 500) && (x < 500)) {
        expectedValue = 10.0f; // Tile 1 (NW)
      } else if ((y < 500) && (x >= 500)) {
        expectedValue = 20.0f; // Tile 2 (NE)
      } else if ((y >= 500) && (x < 500)) {
        expectedValue = 30.0f; // Tile 3 (SW)
      } else if ((y >= 500) && (x >= 500)) {
        expectedValue = 40.0f; // Tile 4 (SE)
      }

      if (!Arith::feq(dstData[y][x], expectedValue)) {
        match = false;
        BOOST_ERROR("Mismatch at y=" << y << ", x=" << x
                                     << " (Expected: " << expectedValue
                                     << ", Got: " << dstData[y][x] << ")");
        break;
      }
    }
    if (!match) { break; }
  }

  BOOST_CHECK(match);
}

BOOST_AUTO_TEST_CASE(TEST_LARGER_INTO_SMALLER_BLIT)
{
  const float latSpacing = 0.01f;
  const float lonSpacing = 0.01f;

  // 1. Create a SMALL Destination Grid (e.g., 100x100)
  const size_t dstLats = 100;
  const size_t dstLons = 100;
  const float dstNWLat = 40.0f;
  const float dstNWLon = -90.0f;

  LLCoverageArea dstArea(dstNWLat, dstNWLon,
    dstNWLat - (dstLats * latSpacing),
    dstNWLon + (dstLons * lonSpacing),
    latSpacing, lonSpacing, dstLons, dstLats);

  auto outGrid = LatLonGrid::Create("Final", "dBZ", Time::CurrentTime(), dstArea);

  outGrid->getFloat2D()->fill(Constants::DataUnavailable);

  // 2. Create a LARGE Source Grid (e.g., 300x300) that fully encapsulates the small one
  const size_t srcLats = 300;
  const size_t srcLons = 300;
  // Push the NW corner further North and West by 100 pixels
  const float srcNWLat = dstNWLat + (100 * latSpacing);
  const float srcNWLon = dstNWLon - (100 * lonSpacing);

  LLCoverageArea srcArea(srcNWLat, srcNWLon,
    srcNWLat - (srcLats * latSpacing),
    srcNWLon + (srcLons * lonSpacing),
    latSpacing, lonSpacing, srcLons, srcLats);

  auto largeSource = LatLonGrid::Create("Source", "dBZ", Time::CurrentTime(), srcArea);

  // Fill the large source grid with a specific value (99.0f)
  largeSource->getFloat2D()->fill(99.0f);

  // 3. Perform the Blit (Copying the 300x300 into the 100x100)
  BOOST_REQUIRE(largeSource->OverlayAligned(outGrid));

  // 4. Verify the entire 100x100 destination grid was filled with 99.0f
  auto& dstData = outGrid->getFloat2DRef();
  bool match    = true;

  for (size_t y = 0; y < dstLats; ++y) {
    for (size_t x = 0; x < dstLons; ++x) {
      if (!Arith::feq(dstData[y][x], 99.0f)) {
        match = false;
        BOOST_ERROR("Destination grid was not properly filled at y=" << y << ", x=" << x
                                                                     << " (Expected: 99.0, Got: " << dstData[y][x] <<
          ")");
        break;
      }
    }
    if (!match) { break; }
  }

  BOOST_CHECK(match);
}

BOOST_AUTO_TEST_SUITE_END()
