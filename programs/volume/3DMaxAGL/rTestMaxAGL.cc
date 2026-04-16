#include "rMaxAGL.h"
#include <iostream>
#include <cassert>
#include "rLatLonHeightGrid.h"
#include "rLatLonGrid.h"
#include "rConstants.h"
#include "rTime.h"
#include <memory>

using namespace rapio;

// Subclass MaxAGL to inject test data and bypass reading from disk
class TestMaxAGLAlg : public MaxAGL {
public:
  TestMaxAGLAlg()
  {
    // Set the search height limit directly (e.g., up to 10 km AGL)
    myTopAglKMs = 10.0f;
  }

  // Expose the protected processVolume method
  void
  testProcess(std::shared_ptr<LatLonHeightGrid> input)
  {
    setupVolumeProcessing(input);
    processVolume(input, this);
  }

  // Expose the protected output grid
  std::shared_ptr<LatLonGrid>
  getOutputGrid()
  {
    return myMaxGrid;
  }
};

class SyntheticCube {
public:
  static std::shared_ptr<LatLonHeightGrid>
  createEmptyCube(size_t nx, size_t ny, size_t nz)
  {
    // Create a 3D grid starting at 35 N, -98 W with 0.1 degree spacing
    auto cube = LatLonHeightGrid::Create("Reflectivity", "dBZ",
        LLH(35.0, -98.0, 0),
        Time::CurrentTime(),
        0.1f, 0.1f, ny, nx, nz);

    // Set layer heights (0km, 1km, 2km...) in meters
    for (size_t z = 0; z < nz; ++z) {
      cube->setLayerValue(z, z * 1000);
    }

    // Fill out the 3D space with MissingData
    auto& data3D = cube->getFloat3DRef();

    for (size_t z = 0; z < nz; ++z) {
      for (size_t y = 0; y < ny; ++y) {
        for (size_t x = 0; x < nx; ++x) {
          data3D[z][y][x] = Constants::MissingData;
        }
      }
    }
    return cube;
  }

  static std::shared_ptr<LatLonGrid>
  createZeroTerrain(size_t nx, size_t ny)
  {
    // Create a matching 2D grid for terrain
    auto terr = LatLonGrid::Create("DEM", "Meters",
        LLH(35.0, -98.0, 0),
        Time::CurrentTime(),
        0.1f, 0.1f, ny, nx);
    auto& data2D = terr->getFloat2DRef();

    for (size_t y = 0; y < ny; ++y) {
      for (size_t x = 0; x < nx; ++x) {
        data2D[y][x] = 0.0f; // Everything is at sea level for the test
      }
    }
    return terr;
  }
};

int
mainold(int argc, char ** argv)
{
  std::cout << "--- Starting MaxAGL Unit Test ---" << std::endl;

  size_t nx = 10, ny = 10, nz = 20;
  auto testCube = SyntheticCube::createEmptyCube(nx, ny, nz);
  auto testTerr = SyntheticCube::createZeroTerrain(nx, ny);

  auto& data3D = testCube->getFloat3DRef();

  // Plant values. Note dimensions for RAPIO are [z][y][x] where y=lat, x=lon

  // Storm 1 at (y=5, x=5)
  data3D[5][5][5] = 50.0f; // At 5km height, value 50
  data3D[8][5][5] = 45.0f; // At 8km height, value 45

  // Storm 2 at (y=2, x=2)
  data3D[3][2][2] = 40.0f; // At 3km height, value 40
  data3D[9][2][2] = 10.0f; // At 9km height, value 10

  // Storm 3 at (y=7, x=7) - Checking the AGL height bounds limit
  data3D[2][7][7]  = 20.0f; // At 2km height, value 20 (Should be max since it's < 10km AGL)
  data3D[15][7][7] = 60.0f; // At 15km height, value 60 (Should be ignored, > 10km AGL limit)

  // Initialize our test algorithm
  auto maxAglAlg = std::make_shared<TestMaxAGLAlg>();

  maxAglAlg->setTerrainData(testTerr);

  // Process the volume
  maxAglAlg->testProcess(testCube);

  // Evaluate constraints
  auto result2D = maxAglAlg->getOutputGrid();

  if (result2D) {
    auto& outData = result2D->getFloat2DRef();

    float h1 = outData[5][5];
    float h2 = outData[2][2];
    float h3 = outData[7][7];
    float h4 = outData[0][0]; // Should remain missing data

    std::cout << "Storm 1 (Expected 50.0)    : " << h1 << std::endl;
    std::cout << "Storm 2 (Expected 40.0)    : " << h2 << std::endl;
    std::cout << "Storm 3 (Expected 20.0)    : " << h3 << std::endl;
    std::cout << "Empty   (Expected Missing) : " << h4 << std::endl;

    assert(h1 == 50.0f);
    assert(h2 == 40.0f);
    assert(h3 == 20.0f);
    assert(h4 == Constants::MissingData);

    std::cout << "SUCCESS: All tests passed!" << std::endl;
    return 0;
  } else {
    std::cerr << "FAIL: No 2D grid produced." << std::endl;
    return 1;
  }
} // main

int
main(int argc, char ** argv)
{
  std::cout << "--- Starting MaxAGL Unit Test ---" << std::endl;
  size_t nx = 10, ny = 10, nz = 20;

  auto testCube = SyntheticCube::createEmptyCube(nx, ny, nz);
  auto testTerr = SyntheticCube::createZeroTerrain(nx, ny);

  // --- Populate 3D Data ---
  auto& data3D = testCube->getFloat3DRef();

  // Point 1: [5][5]
  data3D[5][5][5] = 50.0f; // 5 km MSL
  data3D[8][5][5] = 45.0f; // 8 km MSL

  // Point 2: [2][2]
  data3D[3][2][2] = 40.0f; // 3 km MSL
  data3D[9][2][2] = 10.0f; // 9 km MSL

  // Point 3: [7][7]
  data3D[2][7][7]  = 20.0f; // 2 km MSL
  data3D[15][7][7] = 60.0f; // 15 km MSL

  // --- Modify Terrain ---
  auto& terr2D = testTerr->getFloat2DRef();

  // Elevate terrain at [5][5] to 6,000 meters (6 km).
  // The 10km AGL window is now 6 km to 16 km MSL.
  // The 5km/50.0f value is now underground and should be ignored.
  // The 8km/45.0f value is now at 2km AGL. Expected Max: 45.0f.
  terr2D[5][5] = 6000.0f;

  // Elevate terrain at [7][7] to 6,000 meters (6 km).
  // The 10km AGL window is now 6 km to 16 km MSL.
  // The 2km/20.0f value is underground.
  // The 15km/60.0f value is now at 9km AGL and gets included! Expected Max: 60.0f.
  terr2D[7][7] = 6000.0f;

  // Terrain at [2][2] remains 0 meters.
  // The 10km AGL window is 0 km to 10 km MSL.
  // Both 3km and 9km are included. Expected Max: 40.0f.

  auto maxAglAlg = std::make_shared<TestMaxAGLAlg>();

  // Assuming you made setTerrainData public in VolumeAlgorithm as discussed
  maxAglAlg->setTerrainData(testTerr);
  maxAglAlg->testProcess(testCube);

  auto result2D = maxAglAlg->getOutputGrid();

  if (result2D) {
    auto& outData = result2D->getFloat2DRef();
    float h1      = outData[5][5];
    float h2      = outData[2][2];
    float h3      = outData[7][7];
    float h4      = outData[0][0];
    std::cout << "Storm 1 (Terrain 6km, Expected 45.0) : " << h1 << std::endl;
    std::cout << "Storm 2 (Terrain 0km, Expected 40.0) : " << h2 << std::endl;
    std::cout << "Storm 3 (Terrain 6km, Expected 60.0) : " << h3 << std::endl;
    std::cout << "Empty   (Expected Missing)           : " << h4 << std::endl;

    // FIX: Hard conditionals instead of NDEBUG-vulnerable asserts
    bool passed = true;
    if (h1 != 45.0f) { std::cerr << "FAIL: Storm 1 mismatch." << std::endl; passed = false; }
    if (h2 != 40.0f) { std::cerr << "FAIL: Storm 2 mismatch." << std::endl; passed = false; }
    if (h3 != 60.0f) { std::cerr << "FAIL: Storm 3 mismatch." << std::endl; passed = false; }
    if (h4 != Constants::MissingData) { std::cerr << "FAIL: Empty storm mismatch." << std::endl; passed = false; }

    if (passed) {
      std::cout << "SUCCESS: All tests passed with terrain variations!" << std::endl;
      return 0;
    } else {
      std::cerr << "TEST FAILED!" << std::endl;
      return 1;
    }
  } else {
    std::cerr << "FAIL: No 2D grid produced." << std::endl;
    return 1;
  }
} // main
