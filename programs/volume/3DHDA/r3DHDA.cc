#include "r3DHDA.h"
#include "rIODataType.h"
#include "rLatLonHeightGridIterator.h"
#include "rDataTypeHistory.h"
#include "rDataTypeGroup.h"
#include "rDataProjection.h"
#include <algorithm>
#include <cmath>

using namespace rapio;

extern "C" {
RAPIOAlgorithm *
createRAPIOAlg(void)
{
  return reinterpret_cast<RAPIOAlgorithm *>(new rapio::HDAAlg());
}
}

namespace {
constexpr float kZl             = 40.0f;
constexpr float kZu             = 50.0f;
constexpr float kPoshSlopeA     = 0.0575f;
constexpr float kPoshInterceptB = -121.0f;
constexpr float kMinWT          = 20.0f;
constexpr float kDegToRad       = 3.14159265358979323846f / 180.0f;

inline float
computeShiIncrement(
  float dbz,
  float heightM,
  float hdiffM,
  float h273M,
  float h253M)
{
  if ((dbz <= kZl) || (heightM <= h273M) || (hdiffM <= 0.0f) ) {
    return 0.0f;
  }

  float shiExtra = hdiffM;

  if ((heightM < h253M) && (h253M > h273M) ) {
    shiExtra *= (heightM - h273M) / (h253M - h273M);
  }
  shiExtra *= std::pow(10.0f, 0.084f * dbz);
  if (dbz < kZu) {
    shiExtra *= (dbz - kZl) / (kZu - kZl);
  }
  shiExtra *= 0.000005f;
  return shiExtra;
}

inline void
finalizeHailColumn(
  float rawShi,
  float wt,
  float background,
  float & outShi,
  float & outPosh,
  float & outMesh)
{
  if (rawShi <= 1.0f) {
    outShi = background;
  } else {
    outShi = 0.1f * rawShi;
  }

  if (outShi <= 0.0f) {
    outPosh = background;
    outMesh = background;
    return;
  }

  float posh = 29.0f * std::log(outShi / wt) + 50.0f;

  if (posh <= 0.0f) {
    outPosh = background;
  } else if (posh >= 100.0f) {
    outPosh = 100.0f;
  } else {
    outPosh = posh;
  }

  float mesh = 2.54f * std::sqrt(outShi);

  outMesh = (mesh <= 0.0f) ? background : mesh;
}
} // namespace

namespace rapio {
HDAAlg::HDAAlg() : VolumeAlgorithm("3DHDA")
{ }

void
HDAAlg::declareOptions(RAPIOOptions& o)
{
  VolumeAlgorithm::declareOptions(o);
  o.setDescription("Calculates Hail Diagnosis products (SHI, POSH, MESH) from 3D Reflectivity cubes.");
  o.setAuthors("Robert Toomey, Valliappa Lakshmanan (Lak)");
  o.optional("tilt", "false", "Enable optional storm-tilt adjusted hail products (requires SlopeWE/SlopeNS)");
  o.optional("h273name", "Heightof0C", "NSE product name for 273K (0C) height in meters");
  o.optional("h253name", "Heightof-20C", "NSE product name for 253K (-20C) height in meters");
  o.optional("terrainname", "HeightTerrain", "NSE product name for terrain height in meters");
  o.optional("def273", "4000.0", "Default 273K height in meters if NSE grid is unavailable");
  o.optional("def253", "7000.0", "Default 253K height in meters if NSE grid is unavailable");
  declareProduct("SHI", "Severe Hail Index");
  declareProduct("POSH", "Probability of Severe Hail");
  declareProduct("MESH", "Maximum Expected Severe Hail Size");
}

void
HDAAlg::processOptions(RAPIOOptions& o)
{
  VolumeAlgorithm::processOptions(o);
  myDoTilt      = o.getBoolean("tilt");
  myH273Name    = o.getString("h273name");
  myH253Name    = o.getString("h253name");
  myTerrainName = o.getString("terrainname");
  myDef273      = o.getFloat("def273");
  myDef253      = o.getFloat("def253");

  if (myDoTilt) {
    declareProduct("SHI_tilted", "Severe Hail Index (storm-tilt adjusted)");
    declareProduct("POSH_tilted", "Probability of Severe Hail (storm-tilt adjusted)");
    declareProduct("MESH_tilted", "Maximum Expected Severe Hail Size (storm-tilt adjusted)");
  }
}

void
HDAAlg::checkOutputGrids(std::shared_ptr<LatLonHeightGrid> input)
{
  const Time forTime = input->getTime();

  if ((myShigrid == nullptr) || checkCoverageChange(input)) {
    fLogInfo("Creating new output grids for 3DHDA...");
    const auto coverage = input->getLLCoverageArea();

    myShigrid  = LatLonGrid::Create("SHI", "dimensionless", forTime, coverage);
    myPoshgrid = LatLonGrid::Create("POSH", "%", forTime, coverage);
    myMeshgrid = LatLonGrid::Create("MESH", "mm", forTime, coverage);

    myShigrid->setDataAttributeValue("SubType", "");
    myPoshgrid->setDataAttributeValue("SubType", "");
    myMeshgrid->setDataAttributeValue("SubType", "");

    if (myDoTilt) {
      myShiTiltedGrid  = LatLonGrid::Create("SHI_tilted", "dimensionless", forTime, coverage);
      myPoshTiltedGrid = LatLonGrid::Create("POSH_tilted", "%", forTime, coverage);
      myMeshTiltedGrid = LatLonGrid::Create("MESH_tilted", "mm", forTime, coverage);

      myShiTiltedGrid->setDataAttributeValue("SubType", "");
      myShiTiltedGrid->setDataAttributeValue("ColorMap", "SHI");
      myPoshTiltedGrid->setDataAttributeValue("SubType", "");
      myPoshTiltedGrid->setDataAttributeValue("ColorMap", "POSH");
      myMeshTiltedGrid->setDataAttributeValue("SubType", "");
      myMeshTiltedGrid->setDataAttributeValue("ColorMap", "MESH");
    }
  }
  myShigrid->setTime(forTime);
  myPoshgrid->setTime(forTime);
  myMeshgrid->setTime(forTime);

  if (myDoTilt && myShiTiltedGrid) {
    myShiTiltedGrid->setTime(forTime);
    myPoshTiltedGrid->setTime(forTime);
    myMeshTiltedGrid->setTime(forTime);
  }
} // HDAAlg::checkOutputGrids

class HDACallback : public LatLonHeightGridCallback {
public:
  HDACallback(
    std::shared_ptr<DataProjection> proj273,
    std::shared_ptr<DataProjection> proj253,
    std::shared_ptr<DataProjection> projTerrain,
    std::shared_ptr<DataProjection> projSlopeWE,
    std::shared_ptr<DataProjection> projSlopeNS,
    float def273, float def253, float defTerrain,
    bool doTilt,
    ArrayFloat2DPtr shiOut, ArrayFloat2DPtr poshOut, ArrayFloat2DPtr meshOut,
    ArrayFloat2DPtr shiTiltOut  = nullptr,
    ArrayFloat2DPtr poshTiltOut = nullptr,
    ArrayFloat2DPtr meshTiltOut = nullptr)
    : myProj273(proj273),
    myProj253(proj253),
    myProjTerrain(projTerrain),
    myProjSlopeWE(projSlopeWE),
    myProjSlopeNS(projSlopeNS),
    myDef273(def273),
    myDef253(def253),
    myDefTerrain(defTerrain),
    myDoTilt(doTilt),
    myShi(shiOut),
    myPosh(poshOut),
    myMesh(meshOut),
    myShiTilt(shiTiltOut),
    myPoshTilt(poshTiltOut),
    myMeshTilt(meshTiltOut){ }

  void
  handleBeginColumn(LatLonHeightGridIterator * it) override
  {
    myRawShi           = 0.0f;
    myRawShiTilt       = 0.0f;
    myHasAvailableData = false;
    myHasTiltAvailable = false;

    const float atLat = it->getCurrentLatDegs();
    const float atLon = it->getCurrentLonDegs();

    myH273K = myProj273 ? static_cast<float>(myProj273->getValueAtLL(atLat, atLon)) : myDef273;
    if (!Constants::isGood(myH273K)) {
      myH273K = myDef273;
    }

    myH253K = myProj253 ? static_cast<float>(myProj253->getValueAtLL(atLat, atLon)) : myDef253;
    if (!Constants::isGood(myH253K)) {
      myH253K = myDef253;
    }

    float elevation = myDefTerrain;

    if (it->haveTerrain()) {
      elevation = it->getTerrainHeightKMs() * 1000.0f;
    } else if (myProjTerrain) {
      elevation = static_cast<float>(myProjTerrain->getValueAtLL(atLat, atLon));
    }
    if (!Constants::isGood(elevation)) {
      elevation = myDefTerrain;
    }

    myWT = kPoshSlopeA * (myH273K - elevation) + kPoshInterceptB;
    if (myWT < kMinWT) {
      myWT = kMinWT;
    }

    if (myDoTilt) {
      mySlopeE = myProjSlopeWE ? static_cast<float>(myProjSlopeWE->getValueAtLL(atLat, atLon)) : 0.0f;
      mySlopeS = myProjSlopeNS ? static_cast<float>(myProjSlopeNS->getValueAtLL(atLat, atLon)) : 0.0f;
      if (!Constants::isGood(mySlopeE)) { mySlopeE = 0.0f; }
      if (!Constants::isGood(mySlopeS)) { mySlopeS = 0.0f; }

      myPixelsPerKmLat = 111.0f * it->getLatSpacing();
      myPixelsPerKmLon = 111.0f * it->getLonSpacing() * std::cos(atLat * kDegToRad);
    }
  } // handleBeginColumn

  void
  handleVoxel(LatLonHeightGridIterator * it) override
  {
    const float p = it->getValue();

    if (p != Constants::DataUnavailable) {
      myHasAvailableData = true;
    }

    const auto& heights = it->getHeightsKM();
    const size_t z      = it->getCurrentLayerIdx();
    const float hVLev   = heights[z] * 1000.0f;
    const float hdiff   = (z > 0) ? ((heights[z] - heights[z - 1]) * 1000.0f) : (heights[0] * 1000.0f);

    if (Constants::isGood(p)) {
      myRawShi += computeShiIncrement(p, hVLev, hdiff, myH273K, myH253K);
    }

    if (myDoTilt && myShiTilt) {
      const float deltaHtKm = heights[z] - heights[0];
      const int di      = static_cast<int>((mySlopeS * deltaHtKm + 0.5f) * myPixelsPerKmLat);
      const int dj      = static_cast<int>((mySlopeE * deltaHtKm + 0.5f) * myPixelsPerKmLon);
      const float pTilt = it->getRelativeValue(dj, di, 0);
      if (pTilt != Constants::DataUnavailable) {
        myHasTiltAvailable = true;
      }
      if (Constants::isGood(pTilt)) {
        myRawShiTilt += computeShiIncrement(pTilt, hVLev, hdiff, myH273K, myH253K);
      }
    }
  }

  void
  handleEndColumn(LatLonHeightGridIterator * it) override
  {
    const size_t i = it->getCurrentLatIdx();
    const size_t j = it->getCurrentLonIdx();

    const float background = myHasAvailableData ? Constants::MissingData : Constants::DataUnavailable;
    float shi  = background;
    float posh = background;
    float mesh = background;

    finalizeHailColumn(myRawShi, myWT, background, shi, posh, mesh);

    (*myShi)[i][j]  = shi;
    (*myPosh)[i][j] = posh;
    (*myMesh)[i][j] = mesh;

    if (myDoTilt && myShiTilt && myPoshTilt && myMeshTilt) {
      const float tiltBg = myHasTiltAvailable ? Constants::MissingData : Constants::DataUnavailable;
      float shiT         = tiltBg;
      float poshT        = tiltBg;
      float meshT        = tiltBg;
      finalizeHailColumn(myRawShiTilt, myWT, tiltBg, shiT, poshT, meshT);

      (*myShiTilt)[i][j]  = shiT;
      (*myPoshTilt)[i][j] = poshT;
      (*myMeshTilt)[i][j] = meshT;
    }
  }

private:
  std::shared_ptr<DataProjection> myProj273;
  std::shared_ptr<DataProjection> myProj253;
  std::shared_ptr<DataProjection> myProjTerrain;
  std::shared_ptr<DataProjection> myProjSlopeWE;
  std::shared_ptr<DataProjection> myProjSlopeNS;

  float myDef273;
  float myDef253;
  float myDefTerrain;
  bool myDoTilt;

  ArrayFloat2DPtr myShi, myPosh, myMesh;
  ArrayFloat2DPtr myShiTilt, myPoshTilt, myMeshTilt;

  float myH273K           = 0.0f;
  float myH253K           = 0.0f;
  float myWT              = kMinWT;
  float myRawShi          = 0.0f;
  float myRawShiTilt      = 0.0f;
  bool myHasAvailableData = false;
  bool myHasTiltAvailable = false;

  float mySlopeE         = 0.0f;
  float mySlopeS         = 0.0f;
  float myPixelsPerKmLat = 0.0f;
  float myPixelsPerKmLon = 0.0f;
};

std::unique_ptr<LatLonHeightGridCallback>
HDAAlg::createCallback()
{
  auto proj273     = getNSEProjection(myH273Name);
  auto proj253     = getNSEProjection(myH253Name);
  auto projTerrain = getNSEProjection(myTerrainName);
  auto projSlopeWE = myDoTilt ? getNSEProjection(mySlopeWEName) : nullptr;
  auto projSlopeNS = myDoTilt ? getNSEProjection(mySlopeNSName) : nullptr;

  return std::make_unique<HDACallback>(
    proj273, proj253, projTerrain, projSlopeWE, projSlopeNS,
    myDef273, myDef253, myDefTerrain,
    myDoTilt,
    myShigrid->getFloat2DPtr(),
    myPoshgrid->getFloat2DPtr(),
    myMeshgrid->getFloat2DPtr(),
    myShiTiltedGrid ? myShiTiltedGrid->getFloat2DPtr() : nullptr,
    myPoshTiltedGrid ? myPoshTiltedGrid->getFloat2DPtr() : nullptr,
    myMeshTiltedGrid ? myMeshTiltedGrid->getFloat2DPtr() : nullptr
  );
}

void
HDAAlg::writeFinalProducts(RAPIOAlgorithm * writer)
{
  writer->writeOutputProduct("SHI", myShigrid);
  writer->writeOutputProduct("POSH", myPoshgrid);
  writer->writeOutputProduct("MESH", myMeshgrid);

  if (myDoTilt && myShiTiltedGrid) {
    writer->writeOutputProduct("SHI_tilted", myShiTiltedGrid);
    writer->writeOutputProduct("POSH_tilted", myPoshTiltedGrid);
    writer->writeOutputProduct("MESH_tilted", myMeshTiltedGrid);
  }
}
} // namespace rapio
