#include "r3DVil.h"
#include "rLatLonHeightGridIterator.h"
#include "rDataTypeHistory.h"
#include "rDataTypeGroup.h"
#include "rLatLonGrid.h"
#include "rDataProjection.h"

using namespace rapio;

// Library dynamic link to create this factory
extern "C"
{
RAPIOAlgorithm *
createRAPIOAlg(void)
{
  auto * z = new VIL();

  return reinterpret_cast<RAPIOAlgorithm *>(z);
}
}

VIL::VIL() : VolumeAlgorithm("3DVIL")
{
  // Environment grids (NSE) are now managed locally via a DataTypeGroup 
  // and registered with DataTypeHistory for automatic time-purging.
}

class VILCallback : public LatLonHeightGridCallback {
public:
  VILCallback(
    const float * vilWeights,
    std::shared_ptr<DataProjection> proj263,
    std::shared_ptr<DataProjection> proj233,
    float def263, float def233,
    ArrayFloat2DPtr vilOut, ArrayFloat2DPtr vildOut,
    ArrayFloat2DPtr viiOut, ArrayFloat2DPtr gustOut)
    : myVilWeights(vilWeights),
    myProj263(proj263), myProj233(proj233),
    myDef263(def263), myDef233(def233),
    vilgrid(vilOut), vildgrid(vildOut), viigrid(viiOut), maxgust(gustOut){ }

  void
  handleBeginColumn(LatLonHeightGridIterator * it) override
  {
    vil  = 0.0f;
    vii  = 0.0f;
    et18 = 0.0f; // 0 means we haven't found the echo top yet

    float atLat = it->getCurrentLatDegs();
    float atLon = it->getCurrentLonDegs();

    // Query projections directly, falling back to defaults if data is missing or out of bounds
    H263K = myProj263 ? myProj263->getValueAtLL(atLat, atLon) : myDef263;
    if (!Constants::isGood(H263K)) H263K = myDef263;

    H233K = myProj233 ? myProj233->getValueAtLL(atLat, atLon) : myDef233;
    if (!Constants::isGood(H233K)) H233K = myDef233;
  }

  void
  handleVoxel(LatLonHeightGridIterator * it) override
  {
    float v = it->getValue();

    // Calculate thickness of THIS layer using the iterator's height array
    const auto& heights = it->getHeightsKM();
    size_t z    = it->getCurrentLayerIdx();
    float hdiff = 0.0f;

    if (z > 0) {
      hdiff = (heights[z] - heights[z - 1]) * 1000.0f; // Thickness in meters
    } else if (heights.size() > 1) {
      hdiff = (heights[1] - heights[0]) * 1000.0f; // Assume bottom layer matches layer above
    }

    if ((v == Constants::DataUnavailable) || (v == Constants::MissingData)) {
      return;
    }

    float currentH = heights[z] * 1000.0f; // Current layer height in meters

    // Top-down Echo Top detection (nearest neighbor for now)
    if ((et18 == 0.0f) && (v >= 18.0f)) {
      et18 = currentH;
    }

    int vpos = (int) std::fabs(v);
    int vint = std::min(vpos, vil_upper);

    if (vint < 100) {
      vil += myVilWeights[vint] * hdiff;
    }
    if ((vpos < 100) && ((H263K < currentH) && (currentH < H233K))) {
      vii += myVilWeights[vpos] * hdiff;
    }
  } // handleVoxel

  void
  handleEndColumn(LatLonHeightGridIterator * it) override
  {
    size_t i = it->getCurrentLatIdx();
    size_t j = it->getCurrentLonIdx();

    float vild, w;
    float d = Constants::MissingData;

    if (vil <= 0) {
      vil  = d;
      vild = d;
      w    = d;
    } else {
      vil = 0.00000344f * vil;

      if (et18 > 0.0f) {
        vild = vil / (et18 / 1000.0f); // et18 in km for density
        const float et2 = (et18 / 1000.0f) * (et18 / 1000.0f);
        w = std::sqrt(std::max(0.0f, (15.780608f * vil) - (0.0000023810964f * et2)));
      } else {
        vild = d;
        w    = d;
      }
    }

    vii = 0.00000607f * vii;
    if (vii <= 0.5f) { vii = d; }

    // 3. Write to the four output grids
    (*vilgrid)[i][j]  = vil;
    (*vildgrid)[i][j] = vild;
    (*viigrid)[i][j]  = vii;
    (*maxgust)[i][j]  = w;
  } // handleEndColumn

private:
  const int vil_upper = 56;
  const float * myVilWeights;
  std::shared_ptr<DataProjection> myProj263;
  std::shared_ptr<DataProjection> myProj233;
  float myDef263;
  float myDef233;
  ArrayFloat2DPtr vilgrid, vildgrid, viigrid, maxgust;

  float vil, vii, et18;
  float H263K, H233K;
};

void
VIL::declareOptions(RAPIOOptions& o)
{
  VolumeAlgorithm::declareOptions(o); // Gets terrain, general volume alg stuff.

  o.setDescription(
    "RAPIO 3D VIL. Runs on 3D cubes.");
  o.setAuthors("MRMS");
}

void
VIL::processOptions(RAPIOOptions& o)
{
  VolumeAlgorithm::processOptions(o); // Gets terrain, general volume alg stuff.
}

void
VIL::firstDataSetup()
{
  static bool setup = false;

  if (setup) { return; }

  fLogInfo("Precalculating VIL weight table");
  // Precreate the vil weights.  Saves a little time on each volume
  // computes weights for each dBZ values >= 0 based on a lookup table.
  const float puissance = 4.0 / 7.0;

  for (size_t i = 0; i < 100; ++i) {
    float dbzval = (float) i;
    float zval   = pow(10., (dbzval / 10.));
    myVilWeights[i] = pow(zval, puissance);
  }

  // Initialize and register our NSE group using ProductGroup
  // (ProductGroup inherently uses TypeName as the key)
  // Note: EACH 3D plugin holds its own history of NSE independent of other
  // 3D plugins.  We might need to add a global NSE history time higher up. Currently
  // each plugin can do its own duration.
  myNSEGroup = std::make_shared<ProductGroup>("VIL_NSE_Grids", TimeDuration::Minutes(60));
  DataTypeHistory::registerForPurging(myNSEGroup);

  setup = true;
} // VIL::firstDataSetup

void
VIL::processNewData(RAPIOData& d)
{
  // Intercept 2D Environmental grids
  auto llg2d = d.datatype<LatLonGrid>();
  if (llg2d != nullptr) {
    firstDataSetup(); // Ensure the NSE group is ready
    myNSEGroup->addDataType(llg2d);
    return;
  }
  
  // Otherwise, pass it to the base class to handle the 3D cube
  VolumeAlgorithm::processNewData(d);
}

void
VIL::checkOutputGrids(std::shared_ptr<LatLonHeightGrid> input)
{
  auto& llg = *input; // assuming non-null
  const Time forTime = llg.getTime();
  const LLCoverageArea coverage = llg.getLLCoverageArea();

  if (!checkCoverageChange(input) && (myVilGrid != nullptr)) {
    fLogInfo("Using cached grids at time {}", forTime);
    // Update the time to the input for output
    myVilGrid->setTime(forTime);
    myVildGrid->setTime(forTime);
    myViiGrid->setTime(forTime);
    myMaxGust->setTime(forTime);
    return;
  }

  // --------------------------------------------
  // Create the output product grids we generate
  // Thought about using a single grid, but some fields relay on others
  // so we store all of them
  //
  fLogInfo("Coverage has changed, creating new output grids...");
  myVilGrid = LatLonGrid::Create("VIL", "kg/m^2", forTime, coverage);
  myVilGrid->setDataAttributeValue("SubType", "");

  myVildGrid = LatLonGrid::Create("VIL_Density", "g/m^3", forTime, coverage);
  myVildGrid->setDataAttributeValue("SubType", "");

  myViiGrid = LatLonGrid::Create("VII", "kg/m^2", forTime, coverage);
  myViiGrid->setDataAttributeValue("SubType", "");
  myViiGrid->setDataAttributeValue("ColorMap", "VIL");

  myMaxGust = LatLonGrid::Create("MaxGustEstimate", "m/s", forTime, coverage);
  myMaxGust->setDataAttributeValue("ColorMap", "WindSpeed");
  myMaxGust->setDataAttributeValue("SubType", "");
} // VIL::checkOutputGrids

std::unique_ptr<LatLonHeightGridCallback>
VIL::createCallback()
{
  firstDataSetup(); // Ensure the static weight table and NSE group are initialized

  std::shared_ptr<DataProjection> proj263;
  std::shared_ptr<DataProjection> proj233;

  // Extract the latest grids from the managed group
  if (myNSEGroup) {
      auto dt263 = myNSEGroup->getDataType("Heightof-10C");
      if (dt263) proj263 = dt263->getProjection();

      auto dt233 = myNSEGroup->getDataType("Heightof-40C");
      if (dt233) proj233 = dt233->getProjection();
  }

  return std::make_unique<VILCallback>(
    myVilWeights, 
    proj263, proj233,
    5000.0f, 8600.0f, // Defaults
    myVilGrid->getFloat2DPtr(), myVildGrid->getFloat2DPtr(),
    myViiGrid->getFloat2DPtr(), myMaxGust->getFloat2DPtr()
  );
}

void
VIL::writeFinalProducts(RAPIOAlgorithm * writer)
{
  writer->writeOutputProduct(myVilGrid->getTypeName(), myVilGrid);
  writer->writeOutputProduct(myVildGrid->getTypeName(), myVildGrid);
  writer->writeOutputProduct(myViiGrid->getTypeName(), myViiGrid);
  writer->writeOutputProduct(myMaxGust->getTypeName(), myMaxGust);
}
