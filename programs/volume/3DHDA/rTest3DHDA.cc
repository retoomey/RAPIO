#include "r3DHDA.h"
#include "rLatLonHeightGrid.h"
#include "rLatLonGrid.h"
#include "rConstants.h"
#include "rTime.h"
#include <cmath>
#include <iostream>
#include <memory>

using namespace rapio;

class TestHDAAlg : public HDAAlg {
public:
  TestHDAAlg() = default;

  void
  testProcess(std::shared_ptr<LatLonHeightGrid> input)
  {
    setupVolumeProcessing(input);
    processVolume(input, this);
  }

  void
  writeOutputProduct(
    std::string_view /*key*/,
    std::shared_ptr<DataType> /*outputData*/,
    std::map<std::string, std::string>& /*outputParams*/) override { }

  void
  writeOutputProduct(
    std::string_view /*key*/,
    std::shared_ptr<DataType> /*outputData*/) override { }

  std::shared_ptr<LatLonGrid> getSHI(){ return myShigrid; }

  std::shared_ptr<LatLonGrid> getPOSH(){ return myPoshgrid; }

  std::shared_ptr<LatLonGrid> getMESH(){ return myMeshgrid; }
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
    cube->setLayerValue(z, static_cast<float>(z * 1000)); // 1 km spacing (0..14 km)
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

static bool
approxEqual(float a, float b, float tol = 1e-2f)
{
  return std::fabs(a - b) <= tol;
}

int
main(int /*argc*/, char ** /*argv*/)
{
  std::cout << "--- Starting 3DHDA Unit Test ---" << std::endl;

  const size_t nx = 10, ny = 10, nz = 15;
  auto testCube = createEmptyCube(nx, ny, nz);
  auto& data3D  = testCube->getFloat3DRef();

  // Feed 2D NSE grids (H273K = 4000m, H253K = 7000m)
  const auto coverage = testCube->getLLCoverageArea();
  const Time now      = testCube->getTime();

  auto h273Grid = LatLonGrid::Create("Heightof0C", "m", now, coverage);
  auto h253Grid = LatLonGrid::Create("Heightof-20C", "m", now, coverage);
  auto& h273Arr = h273Grid->getFloat2DRef();
  auto& h253Arr = h253Grid->getFloat2DRef();

  for (size_t y = 0; y < ny; ++y) {
    for (size_t x = 0; x < nx; ++x) {
      h273Arr[y][x] = 4000.0f;
      h253Arr[y][x] = 7000.0f;
    }
  }

  auto hdaAlg = std::make_shared<TestHDAAlg>();

  hdaAlg->addNSEGrid(h273Grid);
  hdaAlg->addNSEGrid(h253Grid);

  // Storm core at [5][5]:
  // z=3 (3km, <= H273K): 55 dBZ (ignored because <= H273K)
  // z=5 (5km): 45 dBZ -> WH = (5000-4000)/(7000-4000) = 1/3, WZ = 10^(0.084*45) * (45-40)/10
  // z=8 (8km): 55 dBZ -> WH = 1.0, WZ = 10^(0.084*55)
  data3D[3][5][5] = 55.0f;
  data3D[5][5][5] = 45.0f;
  data3D[8][5][5] = 55.0f;

  // Low reflectivity column at [2][2] (<= 40 dBZ) -> MissingData
  data3D[6][2][2] = 35.0f;

  hdaAlg->testProcess(testCube);

  auto shiGrid  = hdaAlg->getSHI();
  auto poshGrid = hdaAlg->getPOSH();
  auto meshGrid = hdaAlg->getMESH();

  if (!shiGrid || !poshGrid || !meshGrid) {
    std::cerr << "FAIL: One or more output grids were not created." << std::endl;
    return 1;
  }

  // Analytical expected values for [5][5]
  const float raw5        = 1000.0f * (1.0f / 3.0f) * std::pow(10.0f, 0.084f * 45.0f) * 0.5f * 0.000005f;
  const float raw8        = 1000.0f * 1.0f * std::pow(10.0f, 0.084f * 55.0f) * 0.000005f;
  const float expectedShi = 0.1f * (raw5 + raw8);
  const float wt = 0.0575f * 4000.0f - 121.0f; // 109.0
  const float expectedPosh = std::clamp(29.0f * std::log(expectedShi / wt) + 50.0f, 0.0f, 100.0f);
  const float expectedMesh = 2.54f * std::sqrt(expectedShi);

  const float shiVal  = shiGrid->getFloat2DRef()[5][5];
  const float poshVal = poshGrid->getFloat2DRef()[5][5];
  const float meshVal = meshGrid->getFloat2DRef()[5][5];

  const float emptyShi = shiGrid->getFloat2DRef()[0][0];
  const float lowShi   = shiGrid->getFloat2DRef()[2][2];

  std::cout << "Storm SHI  : " << shiVal << " (expected " << expectedShi << ")" << std::endl;
  std::cout << "Storm POSH : " << poshVal << " % (expected " << expectedPosh << ")" << std::endl;
  std::cout << "Storm MESH : " << meshVal << " mm (expected " << expectedMesh << ")" << std::endl;

  bool passed = true;

  if (!approxEqual(shiVal, expectedShi, 0.05f)) {
    std::cerr << "FAIL: SHI mismatch" << std::endl;
    passed = false;
  }
  if (!approxEqual(poshVal, expectedPosh, 0.05f)) {
    std::cerr << "FAIL: POSH mismatch" << std::endl;
    passed = false;
  }
  if (!approxEqual(meshVal, expectedMesh, 0.05f)) {
    std::cerr << "FAIL: MESH mismatch" << std::endl;
    passed = false;
  }
  if (emptyShi != Constants::MissingData) {
    std::cerr << "FAIL: Empty column should be MissingData, got " << emptyShi << std::endl;
    passed = false;
  }
  if (lowShi != Constants::MissingData) {
    std::cerr << "FAIL: Sub-threshold column should be MissingData, got " << lowShi << std::endl;
    passed = false;
  }

  if (passed) {
    std::cout << "SUCCESS: All 3DHDA tests passed!" << std::endl;
    return 0;
  }
  return 1;
} // main
