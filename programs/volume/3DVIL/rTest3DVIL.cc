#include "r3DVil.h"
#include <iostream>
#include <cassert>
#include "rLatLonHeightGrid.h"
#include "rLatLonGrid.h"
#include "rConstants.h"
#include "rTime.h"
#include <memory>

using namespace rapio;

// A test harness that intercepts disk I/O and exposes the generated grids
class TestVILAlg : public VIL {
public:
  TestVILAlg()
  { }

  void
  testProcess(std::shared_ptr<LatLonHeightGrid> input)
  {
    setupVolumeProcessing(input);
    processVolume(input, this);
  }

  // Override the writer so the unit test doesn't dump files to /tmp or the current directory
  void
  writeOutputProduct(const std::string& key, std::shared_ptr<DataType> outputData, std::map<std::string,
    std::string>& outputParams) override
  {
    // Do nothing for unit test
  }

  void
  writeOutputProduct(const std::string& key, std::shared_ptr<DataType> outputData) override
  {
    // Do nothing for unit test
  }

  std::shared_ptr<LatLonGrid> getVIL(){ return myVilGrid; }

  std::shared_ptr<LatLonGrid> getVILD(){ return myVildGrid; }

  std::shared_ptr<LatLonGrid> getVII(){ return myViiGrid; }

  std::shared_ptr<LatLonGrid> getMaxGust(){ return myMaxGust; }
};

class SyntheticCube {
public:
  static std::shared_ptr<LatLonHeightGrid>
  createEmptyCube(size_t nx, size_t ny, size_t nz)
  {
    auto cube = LatLonHeightGrid::Create("Reflectivity", "dBZ",
        LLH(35.0, -98.0, 0),
        Time::CurrentTime(),
        0.1f, 0.1f, ny, nx, nz);

    for (size_t z = 0; z < nz; ++z) {
      cube->setLayerValue(z, z * 1000); // 1km vertical spacing
    }

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
};

int
main(int argc, char ** argv)
{
  std::cout << "--- Starting 3DVIL Unit Test ---" << std::endl;

  size_t nx = 10, ny = 10, nz = 15; // 15km high cube
  auto testCube = SyntheticCube::createEmptyCube(nx, ny, nz);
  auto& data3D  = testCube->getFloat3DRef();

  // Create a fake storm core at [5][5]
  data3D[1][5][5] = 40.0f; // 1km
  data3D[3][5][5] = 50.0f; // 3km
  data3D[6][5][5] = 55.0f; // 6km (Inside the 5000m-8600m VII growth zone!)
  data3D[8][5][5] = 18.0f; // 8km (This should trip the ET18 top-down check)

  auto vilAlg = std::make_shared<TestVILAlg>();

  vilAlg->testProcess(testCube);

  auto vilGrid  = vilAlg->getVIL();
  auto vildGrid = vilAlg->getVILD();
  auto viiGrid  = vilAlg->getVII();
  auto gustGrid = vilAlg->getMaxGust();

  if (vilGrid && vildGrid && viiGrid && gustGrid) {
    float vil_val   = vilGrid->getFloat2DRef()[5][5];
    float vild_val  = vildGrid->getFloat2DRef()[5][5];
    float vii_val   = viiGrid->getFloat2DRef()[5][5];
    float gust_val  = gustGrid->getFloat2DRef()[5][5];
    float empty_vil = vilGrid->getFloat2DRef()[0][0];

    std::cout << "Storm VIL      : " << vil_val << " kg/m^2" << std::endl;
    std::cout << "Storm VIL Dens : " << vild_val << " g/m^3" << std::endl;
    std::cout << "Storm VII      : " << vii_val << " kg/m^2" << std::endl;
    std::cout << "Storm Max Gust : " << gust_val << " m/s" << std::endl;
    std::cout << "Empty VIL      : " << empty_vil << std::endl;

    bool passed = true;

    if (vil_val <= 0.0f) {
      std::cerr << "FAIL: Storm VIL must be > 0.0f (Got: " << vil_val << ")" << std::endl;
      passed = false;
    }
    if (vild_val <= 0.0f) {
      std::cerr << "FAIL: Storm VIL Density must be > 0.0f (Got: " << vild_val << ")" << std::endl;
      passed = false;
    }
    if (vii_val <= 0.0f) {
      std::cerr << "FAIL: Storm VII must be > 0.0f (Got: " << vii_val << ")" << std::endl;
      passed = false;
    }
    if (gust_val <= 0.0f) {
      std::cerr << "FAIL: Storm Max Gust must be > 0.0f (Got: " << gust_val << ")" << std::endl;
      passed = false;
    }
    if (empty_vil != Constants::MissingData) {
      std::cerr << "FAIL: Empty VIL must be MissingData (Got: " << empty_vil << ")" << std::endl;
      passed = false;
    }

    if (passed) {
      std::cout << "SUCCESS: All 3DVIL tests passed!" << std::endl;
      return 0;
    } else {
      std::cerr << "TEST FAILED!" << std::endl;
      return 1;
    }
  } else {
    std::cerr << "FAIL: One or more 2D grids were not produced." << std::endl;
    return 1;
  }
} // main
