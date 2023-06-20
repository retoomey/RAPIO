#include "rFusion2.h"

#include "rStage2Data.h"

using namespace rapio;

/** Merging requires some basic abilities.  This less data we
 * store per node the better due to the massive size
 * 1.  Need to find nodes with times matching for all XYZ
 *     This is for expiring old data at will.
 *     Current: O(N^3) search xyzStorage, delete nodes
 * 2.  Need to find nodes with radar/elevation matching
 *     Do we need elevation angle to actually merge?  It's a lot
 *     of extra data.
 * 3.  Need to iterate fast in x,y,z to get all nodes matching
 *     This is solved by xyz array with N nodes per location.
 */

void
RAPIOFusionTwoAlg::declareOptions(RAPIOOptions& o)
{
  o.setDescription(
    "RAPIO Fusion Stage Two Algorithm.  Gathers data from multiple radars for mergin values");
  o.setAuthors("Robert Toomey");

  // legacy use the t, b and s to define a grid
  o.declareLegacyGrid();
}

/** RAPIOAlgorithms process options on start up */
void
RAPIOFusionTwoAlg::processOptions(RAPIOOptions& o)
{
  o.getLegacyGrid(myFullGrid);
} // RAPIOFusionTwoAlg::processOptions

void
RAPIOFusionTwoAlg::createLLGCache(
  const std::string    & outputName,
  const std::string    & outputUnits,
  const LLCoverageArea & g)
{
  // NOTE: Tried it as a 3D array.  Due to memory fetching, etc. having N 2D arrays
  // turns out to be faster than 1 3D array.
  if (myLLGCache == nullptr) {
    ProcessTimer("Creating initial LLG value cache.");

    myLLGCache = LLHGridN2D::Create(outputName, outputUnits, Time(), g);
    myLLGCache->fillPrimary(Constants::DataUnavailable);
  }
}

void
RAPIOFusionTwoAlg::firstDataSetup(std::shared_ptr<Stage2Data> data)
{
  static bool setup = false;

  if (setup) { return; }

  setup       = true;
  myRadarName = data->getRadarName();
  myTypeName  = data->getTypeName();

  // Generate output name and units.
  // FIXME: More control flags, maybe even name options
  myWriteCAPPIName   = "Fused2" + myTypeName;
  myWriteOutputUnits = data->getUnits();
  if (myWriteOutputUnits.empty()) {
    LogSevere("Units still wonky because of W2 bug, forcing dBZ at moment..\n");
    myWriteOutputUnits = "dBZ";
  }

  // Create working LLG cache CAPPI storage per height level
  // Note this will be full LLG in ram.
  // FIXME: We could try implementing a LLHGridN2D sparsely but then
  // we'd also have to do writer/reader work
  createLLGCache(myWriteCAPPIName, myWriteOutputUnits, myFullGrid);
}

void
RAPIOFusionTwoAlg::processNewData(rapio::RAPIOData& d)
{
  auto data = Stage2Data::receive(d); // Hide internal format in the stage2 data

  LogInfo("Fusion2 Got data, counting for now:\n");

  size_t counter        = 0;
  size_t missingcounter = 0;
  size_t points         = 0;
  size_t total = 0;

  if (data) {
    float v;
    float w;
    short x;
    short y;
    short z;
    std::string name      = data->getRadarName();
    std::string aTypeName = data->getTypeName();

    LogInfo("Incoming stage2 data for " << name << " " << aTypeName << "\n");

    // Initialize everything related to this radar
    firstDataSetup(data);

    // Check if incoming radar/moment matches our single setup, otherwise we'd need
    // all the setup for each radar/moment.  Which we 'could' do later maybe
    if ((myRadarName != name) || (myTypeName != aTypeName)) {
      LogSevere(
        "We are linked to '" << myRadarName << "-" << myTypeName << "', ignoring radar/typename '" << name << "-" << aTypeName <<
          "'\n");
      return;
    }

    while (data->get(v, w, x, y, z)) { // add time, other stuff
      // TODO: point cloud fun stuff....
      if (counter++ < 10) {
        LogInfo("SAMPLE: " << v << ", " << w << ", (" << x << "," << y << "," << z << ")\n");
      }
      if (v == Constants::MissingData) {
        missingcounter++;
      } else {
        points++;
      }
      total++;
    }
    LogInfo("Final size received: " << points << " points, " << missingcounter << " missing.  Total: " << total <<
      "\n");
  }
} // RAPIOFusionTwoAlg::processNewData

int
main(int argc, char * argv[])
{
  RAPIOFusionTwoAlg alg = RAPIOFusionTwoAlg();

  alg.executeFromArgs(argc, argv);
} // main
