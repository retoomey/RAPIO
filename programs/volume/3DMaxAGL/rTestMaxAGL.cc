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

  // Inject our synthetic terrain grid to bypass loadTerrainGrid() file I/O
  void
  setTestTerrain(std::shared_ptr<LatLonGrid> terr)
  {
    myTerrainGrid = terr;
    myTerrainProj = std::make_shared<LatLonGridProjection>(Constants::PrimaryDataName, myTerrainGrid.get());
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
main(int argc, char ** argv)
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

  maxAglAlg->setTestTerrain(testTerr);

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
