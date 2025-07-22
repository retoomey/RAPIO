#include "rPointCloud.h"

#include "rRAPIOPlugin.h"

using namespace rapio;

void
RAPIOPointCloudAlg::declarePlugins()
{
  // Declare plugins here, this will allow the help to show a list
  // of the available plugin names

  // -------------------------------------------------------------
  // Elevation Volume registration
  PluginVolume::declare(this, "volume");

  // -------------------------------------------------------------
  // TerrainBlockage plugin registration and creation
  PluginTerrainBlockage::declare(this, "terrain");
  // TerrainBlockage::introduce("yourterrain", myTerrainClass); To add your own
}

void
RAPIOPointCloudAlg::declareOptions(RAPIOOptions& o)
{
  o.setDescription(
    "RAPIO. Point Cloud Stage One Algorithm.  Designed to run for a single radar/moment.");
  o.setAuthors("Robert Toomey");

  // legacy use the t, b and s to define a grid
  o.declareLegacyGrid();

  // We make end up wanting some of these...

  // o.optional("throttle", "1",
  //  "Skip count for output files to avoid IO spam during testing, 2 is every other file.  The higher the number, the more files are skipped before writing.");
  // o.addGroup("throttle", "debug");

  // Range to use in KMs.  Default is 460.  This determines subgrid and max range of valid
  // data for the radar
  // o.optional("rangekm", "460", "Range in kilometers for radar.");

  // Default sync heartbeat to 30 seconds for writing
  // Format is seconds then mins
  o.setDefaultValue("sync", "*/30 * * * * *");
} // RAPIOPointCloudAlg::declareOptions

/** RAPIOAlgorithms process options on start up */
void
RAPIOPointCloudAlg::processOptions(RAPIOOptions& o)
{
  // Get grid
  o.getLegacyGrid(myFullGrid);

  myWriteStage2Name  = "None";          // will get from first radialset typename
  myWriteOutputUnits = "Dimensionless"; // will get from first radialset units
  #if 0
  myThrottleCount = o.getInteger("throttle");
  if (myThrottleCount > 10) {
    myThrottleCount = 10;
  } else if (myThrottleCount < 1) {
    myThrottleCount = 1;
  }
  myRangeKMs = o.getFloat("rangekm");
  if (myRangeKMs < 50) {
    myRangeKMs = 50;
  } else if (myRangeKMs > 1000) {
    myRangeKMs = 1000;
  }
  LogInfo("Radar range is " << myRangeKMs << " Kilometers.\n");
  #endif // if 0
} // RAPIOPointCloudAlg::processOptions

void
RAPIOPointCloudAlg::firstDataSetup(std::shared_ptr<RadialSet> r, const std::string& radarName,
  const std::string& typeName)
{
  static bool setup = false;

  if (setup) { return; }

  setup = true;
  LogInfo(ColorTerm::green() << ColorTerm::bold() << "---Initial Startup---" << ColorTerm::reset() << "\n");

  // Radar center coordinates
  const LLH center        = r->getRadarLocation();
  const AngleDegs cLat    = center.getLatitudeDeg();
  const AngleDegs cLon    = center.getLongitudeDeg();
  const LengthKMs cHeight = center.getHeightKM();

  // Link to first incoming radar and moment, we will ignore any others from now on
  LogInfo(
    "Linking this algorithm to radar '" << radarName << "' and typename '" << typeName <<
      "' since first pass we only handle 1\n");
  myTypeName    = typeName;
  myRadarName   = radarName;
  myRadarCenter = center;

  LogInfo("Radar center:" << cLat << "," << cLon << " at " << cHeight << " KMs\n");

  // -------------------------------------------------------------
  // Terrain blockage creation
  auto tp = getPlugin<PluginTerrainBlockage>("terrain");

  if (tp) {
    // This is allowed to be nullptr
    myTerrainBlockage = tp->getNewTerrainBlockage(r->getLocation(), myRangeKMs, radarName);
  }

  // -------------------------------------------------------------
  // Elevation volume creation
  auto volp = getPlugin<PluginVolume>("volume");

  if (volp) {
    myElevationVolume = volp->getNewVolume(myRadarName + "_" + myTypeName);
  }

  // Generate output name and units.
  myWriteStage2Name  = "Point" + r->getTypeName();
  myWriteOutputUnits = r->getUnits();

  // -------------------------------------------------------------
  // Binning grid creation
  // Using FullGrid here...kinda big...might be better with subgrid right?
  size_t y = myFullGrid.getNumY(); // lats
  size_t x = myFullGrid.getNumX(); // lons
  size_t z = myFullGrid.getNumZ(); // Hts

  LogInfo("Creating bin grid of size(" << x << ", " << y << ", " << z << ")\n");
  myGridLookup = std::make_shared<GridPointsLookup>(x, y, z);
} // RAPIOPointCloudAlg::firstDataSetup

void
RAPIOPointCloudAlg::bufferToRadialSet(std::shared_ptr<RadialSet> r)
{
  // Keep the projections for this RadialSet in the RadialSet as we buffer
  // the volume within our output timeframe
  // We could use gate/radial indexes but then would be a lot bigger
  // FIXME: is zero allowed with netcdf or does it choke?
  auto& rad         = *r;
  const size_t size = myValues.size();

  rad.addDim("Points", size);

  // I'm just storing in RadialSet to group it all for a debugging output if needed,
  // and arrays are arrays so eh.
  auto& vs = rad.addFloat1DRef("Values", myWriteOutputUnits, { 2 });

  std::copy(myValues.begin(), myValues.end(), vs.data());
  myValues.clear();

  auto& lats = rad.addFloat1DRef("Latitude", "Degrees", { 2 });

  std::copy(myLatDegs.begin(), myLatDegs.end(), lats.data());
  myLatDegs.clear();

  auto& lons = rad.addFloat1DRef("Longitude", "Degrees", { 2 });

  std::copy(myLonDegs.begin(), myLonDegs.end(), lons.data());
  myLonDegs.clear();

  auto& hts = rad.addFloat1DRef("Heights", "Km", { 2 });

  std::copy(myHeightsKMs.begin(), myHeightsKMs.end(), hts.data());
  myHeightsKMs.clear();

  auto& xs = rad.addFloat1DRef("UX", "Dimensionless", { 2 });

  std::copy(myXs.begin(), myXs.end(), xs.data());
  myXs.clear();

  auto& ys = rad.addFloat1DRef("UY", "Dimensionless", { 2 });

  std::copy(myYs.begin(), myYs.end(), ys.data());
  myYs.clear();

  auto& zs = rad.addFloat1DRef("UZ", "Dimensionless", { 2 });

  std::copy(myZs.begin(), myZs.end(), zs.data());
  myZs.clear();
} // RAPIOPointCloudAlg::bufferToRadialSet

void
RAPIOPointCloudAlg::processRadialSet(std::shared_ptr<RadialSet> r)
{
  LogInfo(ColorTerm::green() << ColorTerm::bold() << "---RadialSet---" << ColorTerm::reset() << "\n");

  auto& rad = *r;

  // Need a radar name in data to handle it currently
  std::string name = "UNKNOWN";

  if (!rad.getString("radarName-value", name)) {
    LogSevere("No radar name found in RadialSet, ignoring data\n");
    return;
  }

  // Get the radar name and typename from this RadialSet.
  const std::string aTypeName = rad.getTypeName();
  const std::string aUnits    = rad.getUnits();
  ProcessTimer ingest("Ingest tilt");

  LogInfo(
    rad.getTypeName() << " (" << rad.getNumRadials() << " Radials * " << rad.getNumGates() << " Gates), " <<
      rad.getElevationDegs() << ", Time: " << rad.getTime() << "\n");

  // Initialize everything related to this radar
  firstDataSetup(r, name, aTypeName);

  // Check if incoming radar/moment matches our single setup, otherwise we'd need
  // all the setup for each radar/moment.  Which we 'could' do later maybe
  if ((myRadarName != name) || (myTypeName != aTypeName)) {
    LogSevere(
      "We are linked to '" << myRadarName << "-" << myTypeName << "', ignoring radar/typename '" << name << "-" << aTypeName <<
        "'\n");
    return;
  }

  // Always add to elevation volume
  myElevationVolume->addDataType(r);

  // Every RadialSet will require terrain per gate for filters.
  // Run a polar terrain algorithm where the results are added to the RadialSet as
  // another array.
  if (myTerrainBlockage != nullptr) {
    ProcessTimer terrain("Applying terrain blockage\n");
    myTerrainBlockage->calculateTerrainPerGate(r);
    LogInfo(terrain);
  }

  ProcessTimer gateProjection("Applying gate projection to RadialSet\n");
  // Iterate over each gate, collecting samples...
  const size_t radials   = rad.getNumRadials(); // x
  const size_t gates     = rad.getNumGates();   // y
  const auto elevDegs    = rad.getElevationDegs();
  LengthKMs firstGateKMs = rad.getDistanceToFirstGateM() / 1000.0; // FIXME: units
  LengthKMs gwKMs        = rad.getGateWidthKMs();
  auto& data = rad.getFloat2DRef();
  auto& azs  = rad.getAzimuthRef();
  auto stationLatDegs = myRadarCenter.getLatitudeDeg();
  auto stationLonDegs = myRadarCenter.getLongitudeDeg();
  auto rHtKMs         = myRadarCenter.getHeightKM();
  const float RAD     = 0.01745329251f;
  const double earthR = 6370949.0;
  const auto rLatRad  = RAD * stationLatDegs;
  const auto rLonRad  = RAD * stationLonDegs;

  const bool haveTerrain = rad.haveTerrain();

  // A temp column buffer since final arrays will be static sized to RadialSet...
  myValues.clear();
  myLatDegs.clear();
  myLonDegs.clear();
  myHeightsKMs.clear();
  myXs.clear();
  myYs.clear();
  myZs.clear();

  // Debug max counter output
  size_t counter = 0;
  bool debugging = false;

  // FIXME: RadialSetIterator class might make cleaner code
  // This would be good for our data types to avoid having to track everything ourselves
  size_t at = 0;

  for (size_t r = 0; r < radials; ++r) {
    LengthKMs distanceKMs = firstGateKMs;
    for (size_t g = 0; g < gates; ++g) {
      LengthKMs oHtKMs;
      AngleDegs oLatDeg, oLonDeg;


      // --------------------------------------------
      // Any non-good values, we skip...
      //
      auto& v = data[r][g];
      if (!Constants::isGood(v)) {
        continue;
      }

      // --------------------------------------------
      // Any gates where beam bottom hits terrain, we skip
      // I'm just making this assumption from fusion
      //
      const bool beamHitBottom = haveTerrain ? (rad.getTerrainBeamBottomHitRef()[r][g] != 0) : 0.0;
      if (beamHitBottom) {
        continue;
      }

      // --------------------------------------------
      // For debugging I want smaller files
      if (debugging && (++counter >= 10)) {
        continue;
      }

      // --------------------------------------------
      // Project Azimuth/Range to the Lat Lon Location
      // FIXME: Make a cached version of this to improve sampling speed
      // since I'm sure the math duplicates some with same station
      Project::BeamPath_AzRangeToLatLon(stationLatDegs, stationLonDegs,
        azs[r],      // azimuth degs
        distanceKMs, // rangeKMs
        elevDegs,
        oHtKMs,
        oLatDeg,
        oLonDeg);

      // FIXME: Make part of Project probably
      // Calculate for the gridbox center point based on radar center
      const float oLatRad = RAD * oLatDeg;
      const float oLonRad = RAD * oLonDeg;
      const float x       = fabs(oLonRad - rLonRad) * earthR; // Arc length in meters
      const float y       = fabs(oLatRad - rLatRad) * earthR; // Arc length in meters
      const float z       = fabs(oHtKMs - rHtKMs) * 1000.0;   // Meters
      const float range   = sqrtf(x * x + y * y + z * z);     // Distance formula
      const float ux      = x / range;                        // units 'should' cancel so technically we're dimensionless
      const float uy      = y / range;
      const float uz      = z / range;

      // The columns...I actually think they will want time as well to be honest.
      // FIXME: more fields probably for stage2 to use
      // FIXME: Make a container class for buffering all columns
      myValues.push_back(v);
      myLatDegs.push_back(oLatDeg);
      myLonDegs.push_back(oLonDeg);
      myHeightsKMs.push_back(oHtKMs);
      myXs.push_back(ux);
      myYs.push_back(uy);
      myZs.push_back(uz);

      distanceKMs += gwKMs;
    }
  }
  LogInfo(gateProjection);

  // Copy the current buffer into RadialSet. We store a virtual volume of these.
  bufferToRadialSet(r);

  // If archive mode, write immediately.  Eh do we need a grouping counter?
  // FIXME: We could have a group counter, so say output every 10 tilts..which would
  // group archive
  if (isArchive()) {
    writeCollectedData(r->getTime()); // Use radial set time at moment...direct 1 to 1
  }
} // RAPIOPointCloudAlg::processRadialSet

void
RAPIOPointCloudAlg::processNewData(rapio::RAPIOData& d)
{
  // Look for any data the system knows how to read...
  // auto r = d.datatype<rapio::DataType>();
  auto r = d.datatype<rapio::RadialSet>();

  if (r != nullptr) {
    // Per incoming tilt triggering.  Issue here is if we get a 'burst'
    // say with a full Canada volume then we do a silly amount of
    // duplicate processing since tilts overlap in effective output.
    // So instead, we'll try waiting for a heartbeat...
    // FIXME: note that archive won't work properly.  I'm really leaning
    // towards removal of realtime flags and doing the simulation technique
    // const Time rTime   = r->getTime();
    // processVolume(rTime);
    // Need dirty first in case we try to write after a single radar....
    myDirty++;

    processRadialSet(r);

    #if 0
    // Create another as a test...
    //
    auto another = RadialSet::Create(
      r->getTypeName(),
      r->getUnits(),
      r->getCenterLocation(),
      r->getTime(),
      1.5,
      r->getDistanceToFirstGateM(),
      r->getNumRadials(),
      r->getNumGates());

    another->setRadarName(r->getRadarName());
    auto& v = another->getFloat2DRef(Constants::PrimaryDataName);
    v[0][0] = 123;

    myDirty++;
    processRadialSet(another);
    #endif // if 0
  }
} // RAPIOPointCloudAlg::processNewData

void
RAPIOPointCloudAlg::processHeartbeat(const Time& n, const Time& p)
{
  // LogInfo(ColorTerm::green() << ColorTerm::bold() << "---Heartbeat---" << ColorTerm::fNormal << "\n");

  // FIXME: For moment, we'll write out for each tilt...though it should be an aggregate of
  // everything received in a time window.
  if (isDaemon()) {
    writeCollectedData(p); // n or p is the question...?
  }
}

void
RAPIOPointCloudAlg::writeCollectedData(const Time rTime)
{
  // Don't do anything unless we got some new data since last time
  if (myDirty < 1) { return; }
  myDirty = 0;

  // ----------------------------------------------------------------------------
  // Gather all current RadialSets in volume to output
  // FIXME:We 'could' possible just process here and not save in the RadialSet.
  // I like saving in the RadialSet for now for debugging options
  //

  // We need to process all tilts we currently have in the volume
  // in this time period.  Each RadialSet already has all the information,
  // just need to copy/group into a single output file.

  // FIXME: needs to work with archive properly...for moment we wrap it
  if (isDaemon()) {
    purgeTimeWindow();
  }

  auto volume = myElevationVolume->getVolume();

  // A temp column buffer since final arrays will be static size
  myValues.clear();
  myLatDegs.clear();
  myLonDegs.clear();
  myHeightsKMs.clear();
  myXs.clear();
  myYs.clear();
  myZs.clear();

  for (size_t i = 0; i < volume.size(); ++i) {
    if (auto radialSetPtr = std::dynamic_pointer_cast<RadialSet>(volume[i])) {
      auto& rad = *radialSetPtr;
      LogInfo("----------->Gathering RadialSet at " << rad.getSubType() << " " << rad.getTime().getString() << "\n");

      auto& vs   = rad.getFloat1DRef("Values");
      auto& lats = rad.getFloat1DRef("Latitude");
      auto& lons = rad.getFloat1DRef("Longitude");
      auto& hts  = rad.getFloat1DRef("Heights");
      auto& xs   = rad.getFloat1DRef("UX");
      auto& ys   = rad.getFloat1DRef("UY");
      auto& zs   = rad.getFloat1DRef("UZ");

      // Append each to final output
      myValues.resize(myValues.size() + vs.size());
      std::copy(vs.begin(), vs.end(), myValues.end() - vs.size()); // append

      myLatDegs.resize(myLatDegs.size() + lats.size());
      std::copy(lats.begin(), lats.end(), myLatDegs.end() - lats.size());

      myLonDegs.resize(myLonDegs.size() + lons.size());
      std::copy(lons.begin(), lons.end(), myLonDegs.end() - lons.size());

      myHeightsKMs.resize(myHeightsKMs.size() + hts.size());
      std::copy(hts.begin(), hts.end(), myHeightsKMs.end() - hts.size());

      myXs.resize(myXs.size() + xs.size());
      std::copy(xs.begin(), xs.end(), myXs.end() - xs.size());

      myYs.resize(myYs.size() + ys.size());
      std::copy(ys.begin(), ys.end(), myYs.end() - ys.size());

      myZs.resize(myZs.size() + zs.size());
      std::copy(zs.begin(), zs.end(), myZs.end() - zs.size());
    }
  }

  // Clear the volume for next time....
  myElevationVolume->clearVolume();

  // ----------------------------------------------------------------------------
  // Now output the final storage to a single output file.
  //
  const size_t size = myValues.size();

  // This stuff 'could' be emcapsulated into a class probably.
  if (size < 1) {
    LogInfo("Skipping writing since we have 0 new rows\n");
  } else {
    LogInfo("Current row size is: " << size << "\n");
  }

  // ----------------------------------------------------------------------------
  // Create a generic netcdf DataGrid to hold our stuff for now.
  // Possibly we'll need custom binary format for IO with any stage2 implementation
  // FIXME: -O product flags at some point?
  myCollectedData = DataGrid::Create(myWriteStage2Name,
      myWriteOutputUnits, myRadarCenter, rTime, { size }, { "Rows" });

  // Copy fields to static arrays for writing
  // FIXME: Being able to change dimensions dynamically would be nice...but for now it's static
  // thus the copy.  We'll see how bad it is.
  auto& vs = myCollectedData->addFloat1DRef(Constants::PrimaryDataName, myWriteOutputUnits, { 0 });

  std::copy(myValues.begin(), myValues.end(), vs.data());

  auto& lats = myCollectedData->addFloat1DRef("Latitude", "Degrees", { 0 });

  std::copy(myLatDegs.begin(), myLatDegs.end(), lats.data());

  auto& lons = myCollectedData->addFloat1DRef("Longitude", "Degrees", { 0 });

  std::copy(myLonDegs.begin(), myLonDegs.end(), lons.data());

  // FIXME: Maybe we'll do heights in meters at some point. Not sure how much it matters
  auto& hts = myCollectedData->addFloat1DRef("Heights", "Km", { 0 });

  std::copy(myHeightsKMs.begin(), myHeightsKMs.end(), hts.data());

  auto& xs = myCollectedData->addFloat1DRef("UX", "Dimensionless", { 0 });

  std::copy(myXs.begin(), myXs.end(), xs.data());

  auto& ys = myCollectedData->addFloat1DRef("UY", "Dimensionless", { 0 });

  std::copy(myYs.begin(), myYs.end(), ys.data());

  auto& zs = myCollectedData->addFloat1DRef("UZ", "Dimensionless", { 0 });

  std::copy(myZs.begin(), myZs.end(), zs.data());

  // No data type so generic netcdf writer
  std::map<std::string, std::string> extraParams;

  extraParams["showfilesize"] = "yes";
  writeOutputProduct(myWriteStage2Name, myCollectedData, extraParams);
} // RAPIOPointCloudAlg::writeCurrentData

int
main(int argc, char * argv[])
{
  // Create and run fusion stage 1
  RAPIOPointCloudAlg alg = RAPIOPointCloudAlg();

  alg.executeFromArgs(argc, argv);
}
