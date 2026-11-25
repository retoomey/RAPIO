#include "r3DLayerAverage.h"
#include "rAlgConfigFile.h"
#include "rLatLonHeightGrid.h"
#include "rLatLonGrid.h"
#include "rConstants.h"
#include "rTime.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>

using namespace rapio;

class TestLayerAverageAlg : public LayerAverageAlg {
public:
  TestLayerAverageAlg() = default;

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
    cube->setLayerValue(z, static_cast<float>(z * 1000));
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
approxEqual(float a, float b, float tol = 1e-3f)
{
  return std::fabs(a - b) <= tol;
}

int
main(int /*argc*/, char ** /*argv*/)
{
  std::cout << "--- Starting 3DLayerAverage Unit Test ---" << std::endl;

  const std::string xmlConfig =
    R"XML(
<w2merger>
  <layeraverage nameof3dgrid="Reflectivity" unit="dBZ" colormap="Reflectivity">
    <product name="ReflAt0C" outputheight="HeightAt0C" operation="at">
      <height product="Heightof0C" default="4000"/>
    </product>
    <product name="EchoTop_30" operation="echotop" threshold="30"/>
    <product name="CloudTop" operation="cloudtop"/>
    <product name="Depth_30" operation="depth" threshold="30"/>
    <product name="Depth_30_Above0C" operation="depth" threshold="30">
      <height product="Heightof0C" default="4000"/>
    </product>
    <product name="LayerAvg_0C_20C" outputheight="HeightAvg_0C_20C" operation="avg">
      <height product="Heightof0C" default="4000"/>
      <height product="Heightof-20C" default="6500"/>
    </product>
    <product name="LowestAlt_0C_20C" outputheight="HeightLowest_0C_20C" operation="lowestalt">
      <height product="Heightof0C" default="4000"/>
      <height product="Heightof-20C" default="6500"/>
    </product>
    <product name="MaxRefl_0C_20C" outputheight="HeightMax_0C_20C" operation="max">
      <height product="Heightof0C" default="4000"/>
      <height product="Heightof-20C" default="6500"/>
    </product>
    <product name="MinRefl_0C_20C" outputheight="HeightMin_0C_20C" operation="min">
      <height product="Heightof0C" default="4000"/>
      <height product="Heightof-20C" default="6500"/>
    </product>
  </layeraverage>
</w2merger>
)XML";

  auto alg = std::make_shared<TestLayerAverageAlg>();

  if (!alg->loadConfigFromXMLString(xmlConfig)) {
    std::cerr << "FAIL: Could not parse XML configuration." << std::endl;
    return 1;
  }

  const size_t nx = 10, ny = 10, nz = 10;
  auto cube    = createEmptyCube(nx, ny, nz);
  auto& data3D = cube->getFloat3DRef();

  // Populate storm column at [5][5]
  data3D[1][5][5] = 25.0f; // 1000 m
  data3D[2][5][5] = 35.0f; // 2000 m
  data3D[3][5][5] = 45.0f; // 3000 m
  data3D[4][5][5] = 55.0f; // 4000 m
  data3D[5][5][5] = 40.0f; // 5000 m
  data3D[6][5][5] = 20.0f; // 6000 m
  data3D[7][5][5] = 10.0f; // 7000 m

  // Feed 2D NSE grid for Heightof0C = 3500 m (Heightof-20C intentionally omitted to test fallback default 6500 m)
  const auto coverage = cube->getLLCoverageArea();
  const Time now      = cube->getTime();
  auto h0C     = LatLonGrid::Create("Heightof0C", "m", now, coverage);
  auto& h0CArr = h0C->getFloat2DRef();

  for (size_t y = 0; y < ny; ++y) {
    for (size_t x = 0; x < nx; ++x) {
      h0CArr[y][x] = 3500.0f;
    }
  }
  alg->addNSEGrid(h0C);

  alg->testProcess(cube);

  bool passed = true;
  auto checkVal = [&](const std::string& name, float expected, size_t y = 5, size_t x = 5) {
      auto grid = alg->getOutput(name);

      if (!grid) {
        std::cerr << "FAIL: Missing output grid '" << name << "'" << std::endl;
        passed = false;
        return;
      }
      float got = grid->getFloat2DRef()[y][x];

      if (!approxEqual(got, expected)) {
        std::cerr << "FAIL: " << name << "[" << y << "][" << x << "] expected "
                  << expected << ", got " << got << std::endl;
        passed = false;
      }
    };

  checkVal("ReflAt0C", 55.0f);
  checkVal("HeightAt0C", 4000.0f);
  checkVal("EchoTop_30", 5.5f);
  checkVal("CloudTop", 7.0f);
  checkVal("Depth_30", 4.0f);
  checkVal("Depth_30_Above0C", 2.0f);
  checkVal("LayerAvg_0C_20C", 115.0f / 3.0f);
  checkVal("HeightAvg_0C_20C", 5000.0f);
  checkVal("LowestAlt_0C_20C", 55.0f);
  checkVal("HeightLowest_0C_20C", 4000.0f);
  checkVal("MaxRefl_0C_20C", 55.0f);
  checkVal("HeightMax_0C_20C", 4000.0f);
  checkVal("MinRefl_0C_20C", 20.0f);
  checkVal("HeightMin_0C_20C", 6000.0f);

  // Empty column [0][0] should be MissingData
  if (auto etGrid = alg->getOutput("EchoTop_30")) {
    if (etGrid->getFloat2DRef()[0][0] != Constants::MissingData) {
      std::cerr << "FAIL: Empty column EchoTop_30 should be MissingData" << std::endl;
      passed = false;
    }
  }

  // --- Test 2: Standalone -iconfig with <w2algxml> containing <option> + <layeraverage> ---
  {
    const std::string singlePath = "/tmp/rapio_test_layeravg_single.xml";
    {
      std::ofstream out(singlePath);
      out <<
        R"XML(
<w2algxml program="LayerAverage">
  <option letter="verbose" value="info" />
  <layeraverage nameof3dgrid="Reflectivity" unit="dBZ" colormap="Reflectivity">
    <product name="EchoTop_30_Single" operation="echotop" threshold="30"/>
  </layeraverage>
</w2algxml>
)XML";
    }
    auto singleAlg = std::make_shared<TestLayerAverageAlg>();
    RAPIOOptions opts("3DLayerAverage");
    singleAlg->declareOptions(opts);
    opts.readConfigFile(singlePath);
    opts.setIsProcessed();
    singleAlg->processOptions(opts);
    singleAlg->testProcess(cube);

    auto etGrid = singleAlg->getOutput("EchoTop_30_Single");
    if (!etGrid || !approxEqual(etGrid->getFloat2DRef()[5][5], 5.5f)) {
      std::cerr << "FAIL: Standalone -iconfig <w2algxml> with <layeraverage> failed." << std::endl;
      passed = false;
    }
    std::remove(singlePath.c_str());
  }

  // --- Test 3: Nested <w2algxml program="LayerAverage"> inside fusionalgs.xml ---
  {
    const std::string fusionPath = "/tmp/rapio_test_fusionalgs.xml";
    {
      std::ofstream out(fusionPath);
      out <<
        R"XML(
<w2algxml>
  <option letter="threads" value="4" />
  <w2algxml program="LayerAverage">
    <layeraverage nameof3dgrid="Reflectivity" unit="dBZ" colormap="Reflectivity">
      <product name="CloudTop_Fused" operation="cloudtop"/>
    </layeraverage>
  </w2algxml>
</w2algxml>
)XML";
    }
    URL fusionUrl(fusionPath);
    auto cfgParser = AlgConfigFile::getConfigFileForURL(fusionUrl);
    std::vector<AlgConfigNode> nodes;
    if (!cfgParser || !cfgParser->readMultiConfigURL(fusionUrl, nodes) || (nodes.size() != 1) || !nodes[0].configNode) {
      std::cerr << "FAIL: readMultiConfigURL did not extract nested LayerAverage configNode." << std::endl;
      passed = false;
    } else {
      auto fusedAlg = std::make_shared<TestLayerAverageAlg>();
      RAPIOOptions scopedOpts(nodes[0].name);
      fusedAlg->declareOptions(scopedOpts);
      scopedOpts.setConfigNode(nodes[0].configNode);
      scopedOpts.setIsProcessed();
      fusedAlg->processOptions(scopedOpts);
      fusedAlg->testProcess(cube);

      auto ctGrid = fusedAlg->getOutput("CloudTop_Fused");
      if (!ctGrid || !approxEqual(ctGrid->getFloat2DRef()[5][5], 7.0f)) {
        std::cerr << "FAIL: Fused fusionalgs.xml nested <layeraverage> failed." << std::endl;
        passed = false;
      }
    }
    std::remove(fusionPath.c_str());
  }

  // --- Test 4: Missing <layeraverage> config throws StartupException ---
  {
    auto emptyAlg = std::make_shared<TestLayerAverageAlg>();
    RAPIOOptions emptyOpts("3DLayerAverage");
    emptyAlg->declareOptions(emptyOpts);
    emptyOpts.setIsProcessed();
    bool threw = false;
    try {
      emptyAlg->processOptions(emptyOpts);
    } catch (const StartupException&) {
      threw = true;
    }
    if (!threw) {
      std::cerr << "FAIL: Expected StartupException when no <layeraverage> config is supplied." << std::endl;
      passed = false;
    }
  }

  // --- Test 5: Legacy <w2merger> translation via readMultiConfigURL ---
  {
    const std::string w2Path = "/tmp/rapio_test_w2merger.xml";
    {
      std::ofstream out(w2Path);
      out <<
        R"XML(
<w2merger>
  <layeraverage nameof3dgrid="Reflectivity" unit="dBZ" colormap="Reflectivity">
    <product name="CloudTop_W2" operation="cloudtop"/>
  </layeraverage>
  <hda>
    <height273 product="Heightof0C" default="3920" />
    <height263 product="Heightof-10C" default="5000" />
    <height253 product="Heightof-20C" default="6200" />
    <height233 product="Heightof-40C" default="8600" />
    <heightTerrain product="TerrainElevation" default="0" />
  </hda>
</w2merger>
)XML";
    }
    URL w2Url(w2Path);
    auto cfgParser = AlgConfigFile::getConfigFileForURL(w2Url);
    std::vector<AlgConfigNode> nodes;
    if (!cfgParser || !cfgParser->readMultiConfigURL(w2Url, nodes) || (nodes.size() != 3)) {
      std::cerr << "FAIL: Expected 3 translated AlgConfigNodes (LayerAverage, HDA, VIL) from <w2merger>, got "
                << nodes.size() << std::endl;
      passed = false;
    }
    std::remove(w2Path.c_str());
  }

  if (passed) {
    std::cout << "PASS: All 3DLayerAverage assertions succeeded." << std::endl;
    return 0;
  }
  return 1;
} // main
