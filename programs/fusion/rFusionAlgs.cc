#include "rFusionAlgs.h"

using namespace rapio;

void
RAPIOFusionAlgs::declareOptions(RAPIOOptions& o)
{
  o.setDescription(
    "RAPIO Fusion Stage Three Algorithm.  Runs algorithms on received 3D cubes.");
  o.setAuthors("Robert Toomey");
}

/** RAPIOAlgorithms process options on start up */
void
RAPIOFusionAlgs::processOptions(RAPIOOptions& o)
{ } // RAPIOFusionAlgs::processOptions

void
RAPIOFusionAlgs::firstDataSetup()
{
  static bool setup = false;

  if (setup) { return; }
  setup = true;


  auto myVIL = std::make_shared<VIL>();

  myVIL->initialize(); // hack for moment

  myAlgs.push_back(myVIL);
}

void
RAPIOFusionAlgs::processNewData(rapio::RAPIOData& d)
{
  fLogInfo("{}{}---Stage3---{}", ColorTerm::green(), ColorTerm::bold(), ColorTerm::reset());
  firstDataSetup();

  // This will read the data even it if isn't the type wanted.  Interesting.
  // In operations this doesn't always matter since we tend to only send wanted
  // data to the algorithm anyway.  Or we're 'supposed' to.
  // Perhaps we can expand record ability to allow more filtering options
  auto llg = d.datatype<rapio::LatLonHeightGrid>();

  if (llg != nullptr) {
    for (auto& a:myAlgs) {
      a->processVolume(llg, this);
    }
  }
} // RAPIOFusionAlgs::processNewData

void
RAPIOFusionAlgs::processHeartbeat(const Time& n, const Time& p)
{
  // Probably don't need heartbeat for the algorithms.  Should be one 3D cube to outputs.
  if (isDaemon()) { // just checking, don't think we get message if we're not
    fLogInfo("{}{}---Heartbeat---{}", ColorTerm::green(), ColorTerm::bold(), ColorTerm::reset());
  }
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
  // FIXME: Wondering if we should just have an array of output LatLonGrids
  // and use enum
  auto& llg = *input; // assuming non-null
  const Time forTime = llg.getTime();
  const LLCoverageArea coverage = llg.getLLCoverageArea();

  auto create = ((myVilGrid == nullptr) || (myCachedCoverage != coverage));

  if (!create) {
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

  myCachedCoverage = coverage;
} // VIL::checkOutputGrids

void
VIL::processVolume(std::shared_ptr<LatLonHeightGrid> input, RAPIOAlgorithm * p)
{
  // Now the real question is WHY can't the VIL algorithm run standalone, taking
  // a 3D cube input?  So it 'should' be a regular algorithm, right?
  // In other words, this could be a processNewData within a regular algorithm
  firstDataSetup();

  auto& llg = *input; // assuming non-null

  #if 0
  myNSEGrid->updateTime(grid.getTime() );
  #endif

  constexpr int vil_upper = 56;

  // --------------------------------------------
  // Gather original grid information and check
  // cached outputs
  //
  const Time forTime = llg.getTime();
  fLogInfo("Computing VIL at {}", forTime);

  const size_t numLats = llg.getNumLats();
  const size_t numLons = llg.getNumLons();
  const size_t numHts  = llg.getNumLayers();
  const auto l         = llg.getLocation();
  const LLCoverageArea coverage = llg.getLLCoverageArea();
  checkOutputGrids(input);

  // Get the primary arrays of the datatypes
  // This differs from MRMS since we can have multiple arrays per DataType
  auto& grid     = input->getFloat3DRef();
  auto& vilgrid  = myVilGrid->getFloat2DRef();
  auto& vildgrid = myVildGrid->getFloat2DRef();
  auto& viigrid  = myViiGrid->getFloat2DRef();
  auto& maxgust  = myMaxGust->getFloat2DRef();

  // FIXME: why copying?
  // Go through the heights and set up the height difference
  // to the next lower level.
  std::vector<float> hlevels; // , height_diff;
  for (size_t h = 0; h < numHts; h++) {
    auto layerValue = llg.getLayerValue(h);
    hlevels.push_back(layerValue);
    // fLogInfo("Pushed back {}", layerValue);
  }
  // fLogSevere("Done with test");
  // grid.setLayerValue(10, 123456); works.
  // vilgrid->setLayerValue(0, 6000); // Should change height, right?
  // p->writeOutputProduct("TESTING", vilgrid);

  #if 0
  const code::LatLonGrid& h263grid = myNSEGrid->getGrid(Height_263K);
  const code::LatLonGrid& h233grid = myNSEGrid->getGrid(Height_233K);
  #endif

  // MRMS maps the nse grids to conus then uses direct index.  Not sure
  // if we'll do that or query directly...
  // FIXME: Really feel like we need a LLG lat/lon iterator class of some sort
  // FIXME: Having to do lat/lon iteration also seems bleh
  AngleDegs deltaLat = llg.getLatSpacing();
  AngleDegs deltaLon = llg.getLonSpacing();
  LLH nw = llg.getTopLeftLocationAt(0, 0);
  AngleDegs startLat = nw.getLatitudeDeg() - (deltaLat / 2.0);  // move south (lat decreasing)
  AngleDegs startLon = nw.getLongitudeDeg() + (deltaLon / 2.0); // move east (lon increasing)
  AngleDegs atLat    = startLat;
  for (size_t i = 0; i < numLats; ++i, atLat -= deltaLat) {
    AngleDegs atLon = startLon;
    for (size_t j = 0; j < numLons; ++j, atLon += deltaLon) {
      // Default data value will be unavailable unless we find a missing or data value
      float d = Constants::DataUnavailable;

      // Compute Echo Top Interpolated (18 dBZ) using utility function
      // This is inline for speed here
      // const float et18 = doEchoTop(grid, 18.0, hlevels, numHts, i, j);
      // const float et18 = 20;
      // const float et18 = doEchoTop(grid, 18.0, hlevels, numHts, i, j);
      // FIXME: This might be slow
      const float et18 = doEchoTop(*input, 18.0, hlevels, numHts, i, j);

      // Query values from NSE grid
      const float H263K = NSEGridHistory::queryGrid(myHeight263, atLat, atLon);
      const float H233K = NSEGridHistory::queryGrid(myHeight233, atLat, atLon);

      // For each height add up the sum parts of VIL and VII
      float vii = 0;
      float vil = 0;

      float lastH = hlevels[0];
      for (size_t k = 0; k < numHts; ++k) {
        // The data value we're interested in
        const float v = grid[k][i][j];

        // Skip on unavailable data...
        if (v == Constants::DataUnavailable) {
          lastH = hlevels[k];
          continue;
        }
        // ...otherwise missing is the new default data value...
        d = Constants::MissingData;

        if (Constants::isGood(v)) {
          const int vpos    = (int) fabs(v);
          const float hdiff = hlevels[k] - lastH; // So k 0 always is diff of zero and no vil sum?? Hummmm

          // Compute VIL sum part (constrained from 0 to the higher of vil_upper or 100)
          int vint = vpos;
          if (vint > vil_upper) { vint = vil_upper; }
          if (vint < 100) {
            vil += myVilWeights[vint] * hdiff;
          }

          // Compute VII sum part (constrained between -10C and -40C altitudes)
          if ((vpos < 100) && ((H263K < hlevels[k]) && (hlevels[k] < H233K))) {
            // vii+=vil_wt[vpos]*height_diff[k];
            vii += myVilWeights[vpos] * hdiff;
          }
        }
        lastH = hlevels[k];
      } // end k loop

      // Toomey: Multiplying by a positive number (..344) before the <= check wastes cycles since
      // it will be negative before and after...so check vil <= 0 first... this also avoids checking
      // vil <= 0 again for the vil density calculation.
      float vild;
      float w;

      if (vil <= 0) {
        // Actually should be unavailable unless height found one not...
        vil  = d;
        vild = d; // Density here is missing as well
        w    = d;
      } else { // vil > 0
        // Compute final VIL (pulled multiplication out of the sum)
        vil = 0.00000344 * vil;
        // KCooper, per Travis - trying to match NMQ VIL?
        // if (vil <= 1) vil = Constants::MissingData;

        // Compute VIL Density.  We already know vil > 0 here, so check valid echo top
        //
        // VILD(g m ^ -3) = 1000 x vil (kgm ^ -2)     1000 * vil (kgm ^ -2)
        //                  --------------------- =  --------------------------  = v/18km
        //                     18 dbz ET (m)           1000* 18 dbz ET (Km)
        // Our echotop is in KM.
        // vild = (et18 > 0.)? vil/et18: d;
        if (et18 > 0.) {
          vild = vil / et18;

          // Compute estimate of potential maximum gust with a descending downdraft
          // NWS SR-136 Stacy R. Stewart
          // https://docs.lib.noaa.gov/noaa_documents/NWS/NWS_SR/TM_NWS_SR_136.pdf
          // max gust estimate = [20 m/s^2 vil - 3.125x10^-6 * et^2]^ .5
          // Toomey: For this pass I'm assuming VIL and EchoTop should be > 0 for a valid
          // value...
          const float et2 = (et18) * (et18); // Kms gives us m/2 at end
          // Stewart: w = ((20.628571 * vil) - (0.000003125*et2));
          // Air Force Air Weather Service numbers, 1996:
          w = ((15.780608 * vil) - (0.0000023810964 * et2));
          w = pow((double) w, (double) 0.5);
        } else {
          vild = d;
          w    = d;
        }
      }

      // Compute VII
      vii = 0.00000607 * vii;
      if (vii <= 0.5) { vii = d; }

      // Fill final grids with final values
      vilgrid[i][j]  = vil;
      vildgrid[i][j] = vild;
      viigrid[i][j]  = vii;
      maxgust[i][j]  = w;
    }
  }

  // Without nse data 'yet'
  p->writeOutputProduct(myVilGrid->getTypeName(), myVilGrid);
  p->writeOutputProduct(myVildGrid->getTypeName(), myVildGrid);
  p->writeOutputProduct(myViiGrid->getTypeName(), myViiGrid);
  p->writeOutputProduct(myMaxGust->getTypeName(), myMaxGust);
} // VIL::processVolume

int
main(int argc, char * argv[])
{
  RAPIOFusionAlgs alg = RAPIOFusionAlgs();

  alg.executeFromArgs(argc, argv);
} // main
