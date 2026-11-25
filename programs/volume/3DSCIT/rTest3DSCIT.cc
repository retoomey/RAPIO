#include "r3DSCIT.h"
#include "rLatLonHeightGrid.h"
#include "rDataTable.h"
#include "rConstants.h"
#include "rTime.h"
#include <cmath>
#include <iostream>
#include <memory>

using namespace rapio;

class TestSCITAlg : public SCITAlg {
public:
  TestSCITAlg() = default;

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

  std::shared_ptr<DataTable> getScitTable(){ return myScitTable; }
};

static std::shared_ptr<LatLonHeightGrid>
createSyntheticCube(size_t nx, size_t ny, size_t nz)
{
  auto cube = LatLonHeightGrid::Create(
    "Reflectivity", "dBZ",
    LLH(35.0, -98.0, 0),
    Time::CurrentTime(),
    0.02f, 0.02f, ny, nx, nz);

  for (size_t z = 0; z < nz; ++z) {
    cube->setLayerValue(z, static_cast<int>(z * 1000));
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

int
main(int /*argc*/, char ** /*argv*/)
{
  std::cout << "--- Starting 3DSCIT Unit Test ---" << std::endl;

  const size_t nx = 20, ny = 20, nz = 5;
  auto testCube = createSyntheticCube(nx, ny, nz);
  auto& data3D  = testCube->getFloat3DRef();

  // 1. Noise spike at z=1, (2,2): 1 pixel (~4.9 km^2 < 15 km^2) -> should be filtered out
  data3D[1][2][2] = 60.0f;

  // 2. Multi-threshold storm cell at z=2 (2000m):
  // Outer 35 dBZ region (y=9..12, x=9..12 -> 4x4 = 16 pixels)
  for (size_t y = 9; y <= 12; ++y) {
    for (size_t x = 9; x <= 12; ++x) {
      data3D[2][y][x] = 35.0f;
    }
  }
  // Inner 50 dBZ core (y=10..11, x=10..11 -> 2x2 = 4 pixels ~ 19.7 km^2 >= 15 km^2)
  data3D[2][10][10] = 50.0f;
  data3D[2][10][11] = 52.0f;
  data3D[2][11][10] = 52.0f;
  data3D[2][11][11] = 58.0f;

  // 3. Separate 35 dBZ cell at z=3 (3000m): 2x2 = 4 pixels (~19.7 km^2 >= 15 km^2)
  for (size_t y = 15; y <= 16; ++y) {
    for (size_t x = 15; x <= 16; ++x) {
      data3D[3][y][x] = 36.0f;
    }
  }

  auto scitAlg = std::make_shared<TestSCITAlg>();

  scitAlg->testProcess(testCube);

  auto table = scitAlg->getScitTable();

  if (!table) {
    std::cerr << "FAIL: Scit2D DataTable was not created." << std::endl;
    return 1;
  }

  if (table->getTypeName() != "Scit2D") {
    std::cerr << "FAIL: Expected TypeName 'Scit2D', got '" << table->getTypeName() << "'" << std::endl;
    return 1;
  }

  if (!table->validateRectangularShape()) {
    std::cerr << "FAIL: DataTable is not rectangular." << std::endl;
    return 1;
  }

  const size_t rowCount = table->getRowCount();

  std::cout << "Detected storm cells: " << rowCount << std::endl;
  if (rowCount != 2) {
    std::cerr << "FAIL: Expected 2 storm cells, got " << rowCount << std::endl;
    return 1;
  }

  const auto& maxDbzCol = table->getColumn("MaxdBZ");
  const auto& threshCol = table->getColumn("ReflectivityThreshold");
  const auto& heightCol = table->getColumn("Height");
  const auto& latCol    = table->getColumn("Latitude");
  const auto& lonCol    = table->getColumn("Longitude");

  bool passed = true;

  // Row 0: z=2 storm core (compressed to 50 dBZ threshold, max 58 dBZ)
  const float max0    = maxDbzCol.getCellAsFloat(0);
  const float thresh0 = threshCol.getCellAsFloat(0);
  const float ht0     = heightCol.getCellAsFloat(0);
  const float lat0    = latCol.getCellAsFloat(0);
  const float lon0    = lonCol.getCellAsFloat(0);

  std::cout << "Cell 0: MaxdBZ=" << max0 << " Thresh=" << thresh0
            << " Height=" << ht0 << " Lat=" << lat0 << " Lon=" << lon0 << std::endl;

  if (std::fabs(max0 - 58.0f) > 0.01f) {
    std::cerr << "FAIL: Cell 0 MaxdBZ expected 58.0, got " << max0 << std::endl;
    passed = false;
  }
  if (std::fabs(thresh0 - 50.0f) > 0.01f) {
    std::cerr << "FAIL: Cell 0 Threshold expected 50.0, got " << thresh0 << std::endl;
    passed = false;
  }
  if (std::fabs(ht0 - 2000.0f) > 0.01f) {
    std::cerr << "FAIL: Cell 0 Height expected 2000.0, got " << ht0 << std::endl;
    passed = false;
  }

  // Row 1: z=3 storm cell (35 dBZ threshold, max 36 dBZ)
  const float max1    = maxDbzCol.getCellAsFloat(1);
  const float thresh1 = threshCol.getCellAsFloat(1);
  const float ht1     = heightCol.getCellAsFloat(1);

  std::cout << "Cell 1: MaxdBZ=" << max1 << " Thresh=" << thresh1
            << " Height=" << ht1 << std::endl;

  if (std::fabs(max1 - 36.0f) > 0.01f) {
    std::cerr << "FAIL: Cell 1 MaxdBZ expected 36.0, got " << max1 << std::endl;
    passed = false;
  }
  if (std::fabs(thresh1 - 35.0f) > 0.01f) {
    std::cerr << "FAIL: Cell 1 Threshold expected 35.0, got " << thresh1 << std::endl;
    passed = false;
  }
  if (std::fabs(ht1 - 3000.0f) > 0.01f) {
    std::cerr << "FAIL: Cell 1 Height expected 3000.0, got " << ht1 << std::endl;
    passed = false;
  }

  if (passed) {
    std::cout << "SUCCESS: All 3DSCIT tests passed!" << std::endl;
    return 0;
  }
  return 1;
} // main
