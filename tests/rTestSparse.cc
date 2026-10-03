#define BOOST_TEST_MODULE SparseTestSuite
#include "rBOOSTTest.h"
#include "rLatLonGrid.h"
#include "rLatLonHeightGrid.h"
#include "rIOConfig.h"
#include "rConstants.h"
#include "rError.h"
#include "rStrings.h"

using namespace rapio;

BOOST_AUTO_TEST_SUITE(SPARSE_TESTS)

BOOST_AUTO_TEST_CASE(TEST_SPARSE_2D)
{
  fLogInfo("--- Testing 2D Sparse/Unsparse (LatLonGrid) ---");
  
  LLH center(35.0, -97.0, 0.0);
  Time t = Time::CurrentTime();
  size_t dimX = 50, dimY = 50;
  auto grid = LatLonGrid::Create("TestGrid2D", "dBZ", center, t, 0.01, 0.01, dimY, dimX);
  
  auto& data = grid->getFloat2DRef();
  float bg = Constants::MissingData;
  
  // Fill completely with background first
  for (size_t x = 0; x < dimX; ++x) {
    for (size_t y = 0; y < dimY; ++y) {
      data[x][y] = bg;
    }
  }
  
  // Add some complex patterns (runs, singles, adjacents)
  data[0][0] = 10.0f; // Single at origin
  data[10][10] = 15.0f; data[10][11] = 15.0f; data[10][12] = 15.0f; // Run of 3
  data[49][49] = 20.0f; // Single at extreme edge
  data[20][49] = 25.0f; data[21][0] = 25.0f; // Run that wraps across the X boundary
  
  auto original = grid->Clone();
  
  IOConfig keys;
  keys.setSparseMode(IOConfig::SparseMode::Force); // Guarantee it sparses
  
  // 1. Sparse it
  BOOST_REQUIRE_MESSAGE(grid->sparse2D(keys), "Failed to sparse 2D grid!");
  
  BOOST_REQUIRE_MESSAGE(grid->getShort1D("pixel_x") && grid->getInt1D("pixel_count"), 
                        "Sparse arrays were not created!");
  
  // 2. DELETE the backup array so it HAS to reconstruct from the RLE data
  grid->deleteArrayName("DisabledPrimary");

  // 3. Unsparse it
  grid->unsparse2D(dimX, dimY, keys, "pixel_x", "pixel_y", "pixel_count");
  
  BOOST_CHECK_MESSAGE(!grid->getShort1D("pixel_x"), "Failed to clean up sparse arrays after decoding!");
  
  // 4. Validate
  auto& decoded = grid->getFloat2DRef();
  auto& orig_data = original->getFloat2DRef();
  
  bool match = true;
  for (size_t x = 0; x < dimX; ++x) {
    for (size_t y = 0; y < dimY; ++y) {
      if (decoded[x][y] != orig_data[x][y]) {
        match = false;
        fLogSevere("Mismatch at [{}][{}]: {} != {}", x, y, decoded[x][y], orig_data[x][y]);
      }
    }
  }
  
  BOOST_CHECK_MESSAGE(match, "Decoded 2D grid does not match original data!");
}

BOOST_AUTO_TEST_CASE(TEST_SPARSE_3D)
{
  fLogInfo("--- Testing 3D Sparse/Unsparse (LatLonHeightGrid) ---");
  
  LLH center(35.0, -97.0, 0.0);
  Time t = Time::CurrentTime();
  size_t dimZ = 10, dimX = 20, dimY = 20;
  auto grid = LatLonHeightGrid::Create("TestGrid3D", "dBZ", center, t, 0.01, 0.01, dimY, dimX, dimZ);
  
  auto& data = grid->getFloat3DRef();
  float bg = Constants::MissingData;
  
  for (size_t z = 0; z < dimZ; ++z) {
    for (size_t x = 0; x < dimX; ++x) {
      for (size_t y = 0; y < dimY; ++y) {
        data[z][x][y] = bg;
      }
    }
  }
  
  // 3D Patterns
  data[0][0][0] = 5.0f; // Origin
  data[5][10][10] = 30.0f; data[5][10][11] = 30.0f; // Simple run
  data[9][19][19] = 50.0f; // Extreme edge
  data[2][19][19] = 40.0f; data[3][0][0] = 40.0f; // Run wrapping across Z layers!
  
  auto original = grid->Clone();
  
  IOConfig keys;
  keys.setSparseMode(IOConfig::SparseMode::Force);
  
  // 1. Sparse it
  BOOST_REQUIRE_MESSAGE(grid->sparse3D(keys), "Failed to sparse 3D grid!");
  
  // 2. Delete backup
  grid->deleteArrayName("DisabledPrimary");

  // 3. Unsparse it
  grid->unsparse3D(dimX, dimY, dimZ, keys, "pixel_x", "pixel_y", "pixel_z", "pixel_count");
  
  // 4. Validate
  auto& decoded = grid->getFloat3DRef();
  auto& orig_data = original->getFloat3DRef();
  
  bool match = true;
  for (size_t z = 0; z < dimZ; ++z) {
    for (size_t x = 0; x < dimX; ++x) {
      for (size_t y = 0; y < dimY; ++y) {
        if (decoded[z][x][y] != orig_data[z][x][y]) {
          match = false;
          fLogSevere("Mismatch at [{}][{}][{}]: {} != {}", z, x, y, decoded[z][x][y], orig_data[z][x][y]);
        }
      }
    }
  }
  
  BOOST_CHECK_MESSAGE(match, "Decoded 3D grid does not match original data!");
}

BOOST_AUTO_TEST_CASE(TEST_ZERO_RUN)
{
  fLogInfo("--- Testing Zero-Run (All Background) Sparsification ---");
  
  LLH center(35.0, -97.0, 0.0);
  Time t = Time::CurrentTime();
  size_t dimX = 10, dimY = 10;
  auto grid = LatLonGrid::Create("TestZeroRun", "dBZ", center, t, 0.01, 0.01, dimY, dimX);
  
  auto& data = grid->getFloat2DRef();
  float bg = Constants::MissingData;
  
  // Grid remains entirely background
  for (size_t x = 0; x < dimX; ++x) {
    for (size_t y = 0; y < dimY; ++y) {
      data[x][y] = bg;
    }
  }
  
  IOConfig keys;
  keys.setSparseMode(IOConfig::SparseMode::Force);
  
  BOOST_REQUIRE_MESSAGE(grid->sparse2D(keys), "Failed to sparse 0-run grid!");
  
  // Verify our dummy pixel was generated
  auto countPtr = grid->getInt1D("pixel_count");
  BOOST_REQUIRE_MESSAGE(countPtr && countPtr->ref().size() == 1, 
                        "0-run grid did not generate exactly 1 dummy run!");
  
  grid->deleteArrayName("DisabledPrimary");
  grid->unsparse2D(dimX, dimY, keys, "pixel_x", "pixel_y", "pixel_count");
  
  auto& decoded = grid->getFloat2DRef();
  bool match = true;
  for (size_t x = 0; x < dimX; ++x) {
    for (size_t y = 0; y < dimY; ++y) {
      if (decoded[x][y] != bg) {
        match = false;
      }
    }
  }
  
  BOOST_CHECK_MESSAGE(match, "0-run unsparse failed! Found non-background data.");
}

BOOST_AUTO_TEST_CASE(TEST_CORRUPT_DATA)
{
  fLogInfo("--- Testing Corrupt Data Protections ---");
  
  LLH center(35.0, -97.0, 0.0);
  Time t = Time::CurrentTime();
  size_t dimX = 10, dimY = 10;
  auto grid = LatLonGrid::Create("TestCorrupt", "dBZ", center, t, 0.01, 0.01, dimY, dimX);
  
  float bg = Constants::MissingData;
  for (size_t x = 0; x < dimX; ++x) {
    for (size_t y = 0; y < dimY; ++y) {
      grid->getFloat2DRef()[x][y] = bg;
    }
  }
  
  grid->getFloat2DRef()[5][5] = 42.0f; // Add one valid run
  
  IOConfig keys;
  keys.setSparseMode(IOConfig::SparseMode::Force);
  grid->sparse2D(keys);
  grid->deleteArrayName("DisabledPrimary");
  
  // INTENTIONALLY CORRUPT THE DATA
  auto& pixel_x = grid->getShort1DRef("pixel_x");
  pixel_x[0] = 9999; // Way out of bounds for a 10x10 grid
  
  fLogInfo("Expect a severe error in the log immediately following this line:");
  grid->unsparse2D(dimX, dimY, keys, "pixel_x", "pixel_y", "pixel_count");
  
  // Because the new unsparseT handles individual bad runs leniently (like legacy),
  // it should complete the unsparse, but SKIP the bad run.
  BOOST_CHECK_MESSAGE(!Strings::beginsWith(grid->getDataType(), "Sparse"), 
                      "Unsparse should complete and remove the 'Sparse' prefix.");
  
  // The resulting grid should be entirely background because the only data run was skipped.
  auto& decoded = grid->getFloat2DRef();
  bool allBackground = true;
  for (size_t x = 0; x < dimX; ++x) {
    for (size_t y = 0; y < dimY; ++y) {
      if (decoded[x][y] != bg) {
        allBackground = false;
      }
    }
  }
  BOOST_CHECK_MESSAGE(allBackground, "Corrupt run was not skipped! Grid contains rogue data.");
}

BOOST_AUTO_TEST_SUITE_END()
