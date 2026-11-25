#include "r3DColumnStats.h"
#include "rLatLonHeightGrid.h"
#include "rLatLonGrid.h"
#include "rConstants.h"
#include "rTime.h"
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <string>

using namespace rapio;

class TestColumnStatsAlg : public ColumnStatsAlg {
public:
  TestColumnStatsAlg() = default;

  void
  testProcess(std::shared_ptr<LatLonHeightGrid> input)
  {
    setupVolumeProcessing(input);
    processVolume(input, this);
  }

  void
  writeOutputProduct(
    std::string_view          key,
    std::shared_ptr<DataType> outputData,
    std::map<std::string, std::string>& /*outputParams*/) override
  {
    writeOutputProduct(key, outputData);
  }

  void
  writeOutputProduct(
    std::string_view          key,
    std::shared_ptr<DataType> outputData) override
  {
    auto llg = std::dynamic_pointer_cast<LatLonGrid>(outputData);

    if (llg) {
      myOutputs[std::string(key)] = llg;
    }
  }

  std::shared_ptr<LatLonGrid>
  getOutput(const std::string& name) const
  {
    auto it = myOutputs.find(name);

    return (it != myOutputs.end()) ? it->second : nullptr;
  }

  size_t
  numOutputs() const { return myOutputs.size(); }

private:
  std::map<std::string, std::shared_ptr<LatLonGrid> > myOutputs;
};

static std::shared_ptr<LatLonHeightGrid>
createEmptyCube(size_t nx, size_t ny, size_t nz)
{
  auto cube = LatLonHeightGrid::Create(
    "Reflectivity", "dBZ",
    LLH(35.0, -98.0, 0),
    Time::CurrentTime(),
    0.1f, 0.1f, ny, nx, nz);

  for (size_t z = 0; z < nz; ++z) {
    cube->setLayerValue(z, static_cast<float>(z * 1000)); // 0..9 km
  }

  auto& data3D = cube->getFloat3DRef();

  for (size_t z = 0; z < nz; ++z) {
    for (size_t y = 0; y < ny; ++y) {
      for (size_t x = 0; x < nx; ++x) {
        data3D[z][y][x] = Constants::DataUnavailable;
      }
    }
  }
  return cube;
}

static bool
approxEqual(float a, float b, float tol = 1e-3f)
{
  return std::fabs(a - b) <= tol;
}

int
main(int /*argc*/, char ** /*argv*/)
{
  std::cout << "--- Starting 3DColumnStats Unit Test ---" << std::endl;

  const size_t nx = 6, ny = 6, nz = 10;
  auto cube    = createEmptyCube(nx, ny, nz);
  auto& data3D = cube->getFloat3DRef();

  // Column [2][2]: mixed positive/negative values to test all 6 ops
  // Layers: z=1 (-70), z=2 (40), z=4 (55), z=6 (65), z=8 (10)
  // max = 65, min = -70, absmax = -70, avg = (-70+40+55+65+10)/5 = 20
  // depth (>=55) = 2 (layers z=4, z=6), lowht (>=55) = 5 (1-based index of z=4)
  data3D[1][2][2] = -70.0f;
  data3D[2][2][2] = 40.0f;
  data3D[4][2][2] = 55.0f;
  data3D[6][2][2] = 65.0f;
  data3D[8][2][2] = 10.0f;

  // Column [3][3]: only MissingData (available radar coverage, no echo)
  data3D[2][3][3] = Constants::MissingData;

  // Column [0][0]: all DataUnavailable

  bool passed = true;

  // 1. Test "all" operations (uncapped full column)
  {
    auto alg = std::make_shared<TestColumnStatsAlg>();
    alg->parseOpsString("all");
    alg->setThreshold(55.0f);
    alg->testProcess(cube);

    auto check = [&](const std::string& prod, size_t y, size_t x, float expected) {
        auto grid = alg->getOutput(prod);

        if (!grid) {
          std::cerr << "FAIL: Missing output product '" << prod << "'" << std::endl;
          passed = false;
          return;
        }
        float got = grid->getFloat2DRef()[y][x];

        if (!approxEqual(got, expected)) {
          std::cerr << "FAIL: " << prod << "[" << y << "][" << x << "] expected "
                    << expected << ", got " << got << std::endl;
          passed = false;
        }
      };

    check("ReflectivityComposite", 2, 2, 65.0f);        // vol_VerticalMaximum
    check("ReflectivityCompositeMin", 2, 2, -70.0f);    // vol_VerticalMinimum
    check("ReflectivityCompositeAbsMax", 2, 2, -70.0f); // vol_VerticalAbsMax
    check("ReflectivityAverage", 2, 2, 20.0f);          // vol_VerticalAverage
    check("ReflectivityDepth", 2, 2, 2.0f);             // vol_VerticalDepth
    check("LowHtRefl", 2, 2, 5.0f);                     // vol_LowHtRefl

    // MissingData vs DataUnavailable propagation
    check("ReflectivityComposite", 3, 3, Constants::MissingData);
    check("ReflectivityComposite", 0, 0, Constants::DataUnavailable);
    check("ReflectivityDepth", 3, 3, Constants::MissingData);
    check("LowHtRefl", 0, 0, Constants::DataUnavailable);
  }

  // 2. Exact parity test matching the retired 3DMaxAGL unit test
  {
    auto maxCube = createEmptyCube(10, 10, 20);
    auto& d3     = maxCube->getFloat3DRef();
    for (size_t z = 0; z < 20; ++z) {
      for (size_t y = 0; y < 10; ++y) {
        for (size_t x = 0; x < 10; ++x) {
          d3[z][y][x] = Constants::MissingData;
        }
      }
    }
    // Point 1: [5][5] with 6000m terrain -> 0-10km AGL is [6km, 16km] -> 5km ignored, 8km (45) kept
    d3[5][5][5] = 50.0f;
    d3[8][5][5] = 45.0f;
    // Point 2: [2][2] with 0m terrain -> 0-10km AGL is [0km, 10km] -> max(40, 10) = 40
    d3[3][2][2] = 40.0f;
    d3[9][2][2] = 10.0f;
    // Point 3: [7][7] with 6000m terrain -> 0-10km AGL is [6km, 16km] -> 2km ignored, 15km (60) kept
    d3[2][7][7]  = 20.0f;
    d3[15][7][7] = 60.0f;

    auto terr  = LatLonGrid::Create("Terrain", "m", maxCube->getTime(), maxCube->getLLCoverageArea());
    auto& tArr = terr->getFloat2DRef();
    for (size_t y = 0; y < 10; ++y) {
      for (size_t x = 0; x < 10; ++x) {
        tArr[y][x] = 0.0f;
      }
    }
    tArr[5][5] = 6000.0f;
    tArr[7][7] = 6000.0f;

    auto alg = std::make_shared<TestColumnStatsAlg>();
    alg->parseOpsString("max");
    alg->setTopAglKMs(10.0f);
    alg->setTerrainData(terr);
    alg->testProcess(maxCube);

    auto maxGrid = alg->getOutput("MaxAGL");
    if (!maxGrid) {
      std::cerr << "FAIL: MaxAGL parity test missing 'MaxAGL' product" << std::endl;
      passed = false;
    } else {
      std::string subType;
      maxGrid->getString("SubType-value", subType);
      if (subType != "0-10.0km") {
        std::cerr << "FAIL: MaxAGL SubType expected '0-10.0km', got '" << subType << "'" << std::endl;
        passed = false;
      }
      auto& out = maxGrid->getFloat2DRef();
      if (!approxEqual(out[5][5], 45.0f) ||
        !approxEqual(out[7][7], 60.0f) ||
        !approxEqual(out[2][2], 40.0f) ||
        (out[0][0] != Constants::MissingData) )
      {
        std::cerr << "FAIL: MaxAGL parity pixel mismatch ("
                  << out[5][5] << ", " << out[7][7] << ", " << out[2][2] << ", " << out[0][0] << ")" << std::endl;
        passed = false;
      }
    }
  }

  // 3. Test selective --ops ("max,avg") with elevated AGL slab (--bottomaglkm 1.0 --topaglkm 3.0)
  {
    auto alg = std::make_shared<TestColumnStatsAlg>();
    alg->parseOpsString("max, avg");
    alg->setBottomAglKMs(1.0f);
    alg->setTopAglKMs(3.0f); // [terrain + 1km, terrain + 3km]

    // Set uniform 2000 m (2 km) terrain -> window is [3km, 5km] (layers z=3..5)
    // In column [2][2], z=2 (40) is below 3km and excluded; only z=4 (55) is inside [3km, 5km]
    auto terrain = LatLonGrid::Create("Terrain", "m", cube->getTime(), cube->getLLCoverageArea());
    auto& tArr   = terrain->getFloat2DRef();
    for (size_t y = 0; y < ny; ++y) {
      for (size_t x = 0; x < nx; ++x) {
        tArr[y][x] = 2000.0f;
      }
    }
    alg->setTerrainData(terrain);
    alg->testProcess(cube);

    if (alg->numOutputs() != 2) {
      std::cerr << "FAIL: Expected exactly 2 outputs for 'max, avg', got " << alg->numOutputs() << std::endl;
      passed = false;
    }

    if (auto maxGrid = alg->getOutput("MaxAGL")) {
      std::string subType;
      maxGrid->getString("SubType-value", subType);
      if (subType != "1.0-3.0km") {
        std::cerr << "FAIL: Slab MaxAGL SubType expected '1.0-3.0km', got '" << subType << "'" << std::endl;
        passed = false;
      }
      float got = maxGrid->getFloat2DRef()[2][2];
      if (!approxEqual(got, 55.0f)) {
        std::cerr << "FAIL: Slab MaxAGL expected 55.0, got " << got << std::endl;
        passed = false;
      }
    } else {
      std::cerr << "FAIL: Missing MaxAGL in AGL slab test" << std::endl;
      passed = false;
    }

    if (auto avgGrid = alg->getOutput("AvgAGL")) {
      float got = avgGrid->getFloat2DRef()[2][2];
      if (!approxEqual(got, 55.0f)) {
        std::cerr << "FAIL: Slab AvgAGL expected 55.0, got " << got << std::endl;
        passed = false;
      }
    } else {
      std::cerr << "FAIL: Missing AvgAGL in AGL slab test" << std::endl;
      passed = false;
    }
  }

  if (passed) {
    std::cout << "PASS: All 3DColumnStats assertions succeeded." << std::endl;
    return 0;
  }
  return 1;
} // main
