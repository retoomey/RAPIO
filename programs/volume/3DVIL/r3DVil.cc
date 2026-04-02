#include "r3DVil.h"
#include "rLatLonHeightGridIterator.h"
#include "rDataTypeHistory.h"
#include "rLatLonGrid.h"

using namespace rapio;

// Library dynamic link to create this factory
extern "C"
{
RAPIOAlgorithm *
createRAPIOAlg(void)
{
  auto * z = new VIL();

  // return reinterpret_cast<void *>(z);
  return reinterpret_cast<RAPIOAlgorithm *>(z);
}
};
// end dynamic module

class VILCallback : public LatLonHeightGridCallback {
public:
  VILCallback(const LatLonHeightGrid& input,
    const float * vilWeights, const std::vector<float>& hls,
    int h263, int h233,
    ArrayFloat2DPtr vilOut, ArrayFloat2DPtr vildOut,
    ArrayFloat2DPtr viiOut, ArrayFloat2DPtr gustOut)
    : inputGrid(input), myVilWeights(vilWeights), hlevels(hls),
    myHeight263(h263), myHeight233(h233),
    vilgrid(vilOut), vildgrid(vildOut), viigrid(viiOut), maxgust(gustOut),
    numHts(hls.size()){ }

  void
  handleBeginColumn(LatLonHeightGridIterator * it) override
  {
    vil   = 0.0f;
    vii   = 0.0f;
    lastH = hlevels[0];
    d     = Constants::DataUnavailable;

    // Query NSE grids at the current column's coordinates
    float atLat = it->getCurrentLatDegs();
    float atLon = it->getCurrentLonDegs();

    H263K = NSEGridHistory::queryGrid(myHeight263, atLat, atLon);
    H233K = NSEGridHistory::queryGrid(myHeight233, atLat, atLon);
  }

  void
  handleVoxel(LatLonHeightGridIterator * it) override
  {
    float v        = it->getValue();
    float currentH = hlevels[it->getCurrentLayerIdx()];

    if (v == Constants::DataUnavailable) {
      lastH = currentH;
      return;
    }

    d = Constants::MissingData; // We have at least one valid/missing voxel

    if (Constants::isGood(v)) {
      int vpos    = (int) std::fabs(v);
      float hdiff = currentH - lastH;

      int vint = std::min(vpos, vil_upper);
      if (vint < 100) {
        vil += myVilWeights[vint] * hdiff;
      }

      if ((vpos < 100) && ((H263K < currentH) && (currentH < H233K))) {
        vii += myVilWeights[vpos] * hdiff;
      }
    }
    lastH = currentH;
  }

  void
  handleEndColumn(LatLonHeightGridIterator * it) override
  {
    size_t i = it->getCurrentLatIdx();
    size_t j = it->getCurrentLonIdx();

    // 1. Calculate EchoTop 18 using the original function
    float et18 = doEchoTop(inputGrid, 18.0, hlevels, numHts, i, j);

    // 2. Finalize VIL, VILD, and Gust calculations
    float vild, w;

    if (vil <= 0) {
      vil  = d;
      vild = d;
      w    = d;
    } else {
      vil = 0.00000344f * vil;
      if (et18 > 0.0f) {
        vild = vil / et18;
        const float et2 = et18 * et18;
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
  const LatLonHeightGrid& inputGrid;
  const float * myVilWeights;
  const std::vector<float>& hlevels;
  int myHeight263, myHeight233;
  ArrayFloat2DPtr vilgrid, vildgrid, viigrid, maxgust;
  size_t numHts;

  float vil, vii, lastH, d;
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
VIL::initialize()
{
  // Directly follow grids at moment.  Probably should call on the
  // start up of the algorithm not at first data.
  // FIXME: configuration at some point
  // NSEGridHistory::followGrid("height273", "Heightof0C", 3920);
  myHeight263 = NSEGridHistory::followGrid("height263", "Heightof-10C", 5000);
  // NSEGridHistory::followGrid("height253", "Heightof-20C", 6200);
  myHeight233 = NSEGridHistory::followGrid("height233", "Heightof-40C", 8600);
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

  #if 0
  NSE will have special expire time in the data I think:
  : ExpiryInterval - unit = "Minutes";
  : ExpiryInterval - value = "1440";
  < hda >
  < height273 product       = "Heightof0C" default = "3920" / >
    < height263 product     = "Heightof-10C" default = "5000" / >
    < height253 product     = "Heightof-20C" default = "6200" / >
    < height233 product     = "Heightof-40C" default = "8600" / >
    < heightTerrain product = "TerrainElevation" default = "0" / >
    < / hda >

    < vilextras >
    < slopeWE product = "Slope_WestToEast" default = "0" note = "For tilted hail/vil products" / >
    < slopeNS product = "Slope_NorthToSouth" default = "0" note = "For tilted hail/vil products" / >
    < meantemp500mb400mb product = "Temp500_400" default = "-15" note =
    "For VIL of day: mean temperature between 500 and 400 mb" / >
    < / vilextras >

  #endif // if 0

  #if 0
  // the doc gives the xml infor for 'what' and a form of group.
  // The group is maybe not neccessary.
  // So when we 'follow' we have a default background value that
  // it provided for the output grid.
  Height_263K = myNSEGrid->followGrid(doc, "hda", "height263");
  Height_233K = myNSEGrid->followGrid(doc, "hda", "height233");
  if (doDilateTilt) {
    SlopeWE  = myNSEGrid->followGrid(doc, "vilextras", "slopeWE");
    SlopeNS  = myNSEGrid->followGrid(doc, "vilextras", "slopeNS");
    MeanTemp = myNSEGrid->followGrid(doc, "vilextras", "meantemp500mb400mb");
  }
  #endif // if 0
  // DataTypeHistory::followDataTypeKey("vileextras");
  // DataTypeHistory::followDataType("hda");
} // VIL::firstDataSetup

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

void
VIL::processVolume(std::shared_ptr<LatLonHeightGrid> input, RAPIOAlgorithm * writer)
{
  // Terrain and Output Grids are handled before we arrive here.
  // We just need to make sure our static weight table is initialized.
  firstDataSetup();

  auto& llg = *input;
  const Time forTime = llg.getTime();

  fLogInfo("Computing VIL at {}", forTime);

  #if 0
  const size_t numHts = llg.getNumLayers();
  std::vector<float> hlevels(numHts);

  for (size_t h = 0; h < numHts; h++) {
    hlevels[h] = llg.getLayerValue(h);
  }
  #endif

  // Set up and run the callback
  VILCallback myCallback(
    llg, myVilWeights, myHLevels, myHeight263, myHeight233,
    myVilGrid->getFloat2DPtr(), myVildGrid->getFloat2DPtr(),
    myViiGrid->getFloat2DPtr(), myMaxGust->getFloat2DPtr()
  );

  LatLonHeightGridIterator iter(llg);

  iter.iterateUpColumns(myCallback);

  // Write outputs
  writer->writeOutputProduct(myVilGrid->getTypeName(), myVilGrid);
  writer->writeOutputProduct(myVildGrid->getTypeName(), myVildGrid);
  writer->writeOutputProduct(myViiGrid->getTypeName(), myViiGrid);
  writer->writeOutputProduct(myMaxGust->getTypeName(), myMaxGust);
} // VIL::processVolume
