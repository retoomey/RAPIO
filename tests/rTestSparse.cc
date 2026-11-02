#define BOOST_TEST_MODULE SparseTestSuite
#include "rBOOSTTest.h"
#include "rLatLonGrid.h"
#include "rLatLonHeightGrid.h"
#include "rLLHGridN2D.h"
#include "rRadialSet.h"
#include "rIOConfig.h"
#include "rConstants.h"
#include "rError.h"
#include "rStrings.h"
#include "rSparseEncoder.h"

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
  float bg   = Constants::MissingData;

  for (size_t x = 0; x < dimX; ++x) {
    for (size_t y = 0; y < dimY; ++y) {
      data[x][y] = bg;
    }
  }

  data[0][0]   = 10.0f;
  data[10][10] = 15.0f;
  data[10][11] = 15.0f;
  data[10][12] = 15.0f;
  data[49][49] = 20.0f;
  data[20][49] = 25.0f;
  data[21][0]  = 25.0f;

  auto original = grid->Clone();

  IOConfig keys;

  keys.setSparseMode(IOConfig::SparseMode::Force);

  BOOST_REQUIRE_MESSAGE(SparseEncoder::sparse2D(*grid, keys), "Failed to sparse 2D grid!");

  BOOST_REQUIRE_MESSAGE(grid->getShort1D("pixel_x") && grid->getInt1D("pixel_count"),
    "Sparse arrays were not created!");

  grid->deleteArrayName("DisabledPrimary");

  SparseEncoder::unsparse2D(*grid, dimX, dimY, keys, "pixel_x", "pixel_y", "pixel_count");

  BOOST_CHECK_MESSAGE(!grid->getShort1D("pixel_x"), "Failed to clean up sparse arrays after decoding!");

  auto& decoded   = grid->getFloat2DRef();
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
  float bg   = Constants::MissingData;

  for (size_t z = 0; z < dimZ; ++z) {
    for (size_t x = 0; x < dimX; ++x) {
      for (size_t y = 0; y < dimY; ++y) {
        data[z][x][y] = bg;
      }
    }
  }

  data[0][0][0]   = 5.0f;
  data[5][10][10] = 30.0f;
  data[5][10][11] = 30.0f;
  data[9][19][19] = 50.0f;
  data[2][19][19] = 40.0f;
  data[3][0][0]   = 40.0f;

  auto original = grid->Clone();

  IOConfig keys;

  keys.setSparseMode(IOConfig::SparseMode::Force);

  BOOST_REQUIRE_MESSAGE(SparseEncoder::sparse3D(*grid, keys), "Failed to sparse 3D grid!");
  grid->deleteArrayName("DisabledPrimary");
  SparseEncoder::unsparse3D(*grid, dimX, dimY, dimZ, keys, "pixel_x", "pixel_y", "pixel_z", "pixel_count");

  auto& decoded   = grid->getFloat3DRef();
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

BOOST_AUTO_TEST_CASE(TEST_SPARSE_RADIALSET)
{
  fLogInfo("--- Testing RadialSet Sparse/Unsparse ---");

  LLH center(35.0, -97.0, 0.0);
  Time t = Time::CurrentTime();
  size_t radials = 360, gates = 1000;

  auto grid = RadialSet::Create("TestRadial", "dBZ", center, t, 0.5f, 2000.0f, 250.0f, radials, gates);

  auto& data = grid->getFloat2DRef();
  float bg   = Constants::MissingData;

  for (size_t r = 0; r < radials; ++r) {
    for (size_t g = 0; g < gates; ++g) {
      data[r][g] = bg;
    }
  }

  for (size_t g = 500; g < 510; ++g) {
    data[90][g] = 45.0f;
  }
  for (size_t g = 800; g < 850; ++g) {
    data[180][g] = 20.0f;
  }

  auto original = grid->Clone();
  IOConfig keys;

  keys.setSparseMode(IOConfig::SparseMode::Force);

  BOOST_REQUIRE_MESSAGE(SparseEncoder::sparse2D(*grid, keys), "Failed to sparse RadialSet!");
  grid->deleteArrayName("DisabledPrimary");
  SparseEncoder::unsparse2D(*grid, radials, gates, keys, "pixel_x", "pixel_y", "pixel_count");

  auto& decoded   = grid->getFloat2DRef();
  auto& orig_data = original->getFloat2DRef();

  bool match = true;

  for (size_t r = 0; r < radials; ++r) {
    for (size_t g = 0; g < gates; ++g) {
      if (decoded[r][g] != orig_data[r][g]) {
        match = false;
        fLogSevere("Radial mismatch at [{}][{}]: {} != {}", r, g, decoded[r][g], orig_data[r][g]);
      }
    }
  }
  BOOST_CHECK_MESSAGE(match, "Decoded RadialSet grid does not match original data!");
}

BOOST_AUTO_TEST_CASE(TEST_RLE_COMPRESSION_LOGIC)
{
  fLogInfo("--- Testing Strict RLE Array Mathematics ---");

  LLH center(35.0, -97.0, 0.0);
  Time t = Time::CurrentTime();
  size_t dimX = 20, dimY = 20;
  auto grid = LatLonGrid::Create("TestRLE", "dBZ", center, t, 0.01, 0.01, dimY, dimX);

  auto& data = grid->getFloat2DRef();
  float bg   = Constants::MissingData;

  for (size_t x = 0; x < dimX; ++x) {
    for (size_t y = 0; y < dimY; ++y) {
      data[x][y] = bg;
    }
  }

  data[0][0] = 10.0f;
  data[2][0] = 20.0f;
  data[2][1] = 20.0f;
  data[2][2] = 20.0f;
  data[2][3] = 20.0f;
  data[2][4] = 20.0f;
  for (size_t y = 5; y < 15; ++y) {
    data[5][y] = 30.0f;
  }

  IOConfig keys;

  keys.setSparseMode(IOConfig::SparseMode::Force);
  SparseEncoder::sparse2D(*grid, keys);

  auto pixelArray = grid->getFloat1D(Constants::PrimaryDataName);
  auto pixelCount = grid->getInt1D("pixel_count");
  auto pixelX     = grid->getShort1D("pixel_x");
  auto pixelY     = grid->getShort1D("pixel_y");

  BOOST_REQUIRE(pixelArray && pixelCount && pixelX && pixelY);

  BOOST_CHECK_EQUAL(pixelArray->ref().size(), 3);
  BOOST_CHECK_EQUAL(pixelCount->ref().size(), 3);

  BOOST_CHECK_EQUAL(pixelArray->ref()[0], 10.0f);
  BOOST_CHECK_EQUAL(pixelCount->ref()[0], 1);

  BOOST_CHECK_EQUAL(pixelArray->ref()[1], 20.0f);
  BOOST_CHECK_EQUAL(pixelCount->ref()[1], 5);

  BOOST_CHECK_EQUAL(pixelArray->ref()[2], 30.0f);
  BOOST_CHECK_EQUAL(pixelCount->ref()[2], 10);
}

BOOST_AUTO_TEST_CASE(TEST_ZERO_RUN)
{
  fLogInfo("--- Testing Zero-Run (All Background) Sparsification ---");

  LLH center(35.0, -97.0, 0.0);
  Time t = Time::CurrentTime();
  size_t dimX = 10, dimY = 10;
  auto grid = LatLonGrid::Create("TestZeroRun", "dBZ", center, t, 0.01, 0.01, dimY, dimX);

  auto& data = grid->getFloat2DRef();
  float bg   = Constants::MissingData;

  for (size_t x = 0; x < dimX; ++x) {
    for (size_t y = 0; y < dimY; ++y) {
      data[x][y] = bg;
    }
  }

  IOConfig keys;

  keys.setSparseMode(IOConfig::SparseMode::Force);

  BOOST_REQUIRE_MESSAGE(SparseEncoder::sparse2D(*grid, keys), "Failed to sparse 0-run grid!");

  auto countPtr = grid->getInt1D("pixel_count");

  BOOST_REQUIRE_MESSAGE(countPtr && countPtr->ref().size() == 1,
    "0-run grid did not generate exactly 1 dummy run!");

  grid->deleteArrayName("DisabledPrimary");
  SparseEncoder::unsparse2D(*grid, dimX, dimY, keys, "pixel_x", "pixel_y", "pixel_count");

  auto& decoded = grid->getFloat2DRef();
  bool match    = true;

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

  grid->getFloat2DRef()[5][5] = 42.0f;

  IOConfig keys;

  keys.setSparseMode(IOConfig::SparseMode::Force);
  SparseEncoder::sparse2D(*grid, keys);
  grid->deleteArrayName("DisabledPrimary");

  auto& pixel_x = grid->getShort1DRef("pixel_x");

  pixel_x[0] = 9999;

  fLogInfo("Expect a severe error in the log immediately following this line:");
  SparseEncoder::unsparse2D(*grid, dimX, dimY, keys, "pixel_x", "pixel_y", "pixel_count");

  BOOST_CHECK_MESSAGE(!Strings::beginsWith(grid->getDataType(), "Sparse"),
    "Unsparse should complete and remove the 'Sparse' prefix.");

  auto& decoded      = grid->getFloat2DRef();
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

BOOST_AUTO_TEST_CASE(TEST_SPARSE_N2D)
{
  fLogInfo("--- Testing N-Stacked 2D Sparse/Unsparse (LLHGridN2D) ---");

  LLH center(35.0, -97.0, 0.0);
  Time t = Time::CurrentTime();
  size_t dimZ = 3, dimX = 10, dimY = 10;

  auto grid = LLHGridN2D::Create("TestGridN2D", "dBZ", t, center, 0.01, 0.01, dimY, dimX, dimZ);

  grid->fillPrimary(Constants::MissingData);

  IOConfig keys;

  keys.setSparseMode(IOConfig::SparseMode::Force);

  fLogInfo("Executing pass 1 (Tiny payload)");
  grid->get(0)->getFloat2DRef()[0][0] = 10.0f;

  grid->preWrite(keys);

  BOOST_REQUIRE_MESSAGE(grid->getDataType() == "SparseLatLonHeightGrid", "Did not rename datatype");
  BOOST_REQUIRE_MESSAGE(grid->getShort1D("pixel_x") != nullptr, "Sparse arrays were not created!");

  grid->postWrite(keys);

  BOOST_REQUIRE_MESSAGE(grid->getDataType() == "LatLonHeightGrid", "Did not restore datatype");
  BOOST_REQUIRE_MESSAGE(grid->getShort1D("pixel_x") == nullptr, "Sparse arrays were not cleaned up!");
  bool hasPixelDim = (!grid->getDims().empty() && grid->getDims().back().name() == "pixel");

  BOOST_REQUIRE_MESSAGE(!hasPixelDim, "Pixel dimension was not popped!");

  fLogInfo("Executing pass 2 (Massive unique payload to defeat RLE and trigger array growth)");

  for (size_t z = 0; z < dimZ; ++z) {
    auto layer = grid->get(z)->getFloat2D();
    for (size_t x = 0; x < dimX; ++x) {
      for (size_t y = 0; y < dimY; ++y) {
        // Assign mathematically unique values so RLE generates exactly 300 runs
        layer->ref()[x][y] = static_cast<float>(z * 100 + x * 10 + y);
      }
    }
  }

  grid->preWrite(keys);

  auto pixelArray = grid->getFloat1D(Constants::PrimaryDataName);

  BOOST_REQUIRE_MESSAGE(
    pixelArray && pixelArray->ref().size() == 300, "Pass 2 array size did not dynamically scale to 300!");

  grid->postWrite(keys);

  fLogInfo("N2D Sparsification passed without memory corruption!");
}

BOOST_AUTO_TEST_SUITE_END()
