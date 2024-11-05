#include <rPolarMerge.h>

#include <iostream>

using namespace rapio;

void
PolarMerge::declarePlugins()
{
  // We'll use virtual volumes and terrain ability
  PluginVolume::declare(this, "volume");
  PluginTerrainBlockage::declare(this, "terrain");
}

void
PolarMerge::declareOptions(RAPIOOptions& o)
{
  o.setDescription(
    "PolarMerge collapses a virtual volume into a single polar product.");
  o.setDefaultValue("terrain", "lak");
}

void
PolarMerge::processOptions(RAPIOOptions& o)
{ }

void
PolarMerge::processNewData(rapio::RAPIOData& d)
{
  // Do we have a radial set then process it
  auto r = d.datatype<rapio::RadialSet>();

  if (r != nullptr) {
    const bool addedTilt = processRadialSet(r);

    // For moment, output after every tilt.
    if (addedTilt) {
      const std::string subtype = "at_" + r->getSubType();
      processVolume(r->getTime(), subtype); // Since just added, use time of incoming
    }
  }
}

bool
PolarMerge::processRadialSet(std::shared_ptr<RadialSet> r)
{
  LogInfo(ColorTerm::fGreen << ColorTerm::fBold << "---RadialSet---" << ColorTerm::fNormal << "\n");
  // Need a radar name in data to handle it currently
  std::string name = "UNKNOWN";

  if (!r->getString("radarName-value", name)) {
    LogSevere("No radar name found in RadialSet, ignoring data\n");
    return false;
  }

  // Get the radar name and typename from this RadialSet.
  const std::string aTypeName = r->getTypeName();
  const std::string aUnits    = r->getUnits();
  const size_t numRadials     = r->getNumRadials();
  const size_t numGates       = r->getNumGates();

  // ProcessTimer ingest("Ingest tilt");

  LogInfo(
    r->getTypeName() << " (" << numRadials << " Radials * " << numGates << " Gates), " <<
      r->getElevationDegs() << ", Time: " << r->getTime() << "\n");

  // Initialize everything related to this radar
  firstDataSetup(r, name, aTypeName);

  // Check if incoming radar/moment matches our single setup, otherwise we'd need
  // all the setup for each radar/moment.  Which we 'could' do later maybe
  if ((myRadarName != name) || (myTypeName != aTypeName)) {
    // LogSevere(
    //   "We are linked to '" << myRadarName << "-" << myTypeName << "', ignoring radar/typename '" << name << "-" << aTypeName <<
    //     "'\n");
    LogInfo("Ignoring radar/typename '" << name << "-" << aTypeName << "'\n");
    return false;
  }

  // Always add to our elevation volume (the virtual volume)
  myElevationVolume->addDataType(r);

  return true;
} // PolarMerge::processRadialSet

void
PolarMerge::firstDataSetup(std::shared_ptr<RadialSet> r, const std::string& radarName,
  const std::string& typeName)
{
  static bool setup = false;

  if (setup) { return; }

  setup = true;
  LogInfo(ColorTerm::fGreen << ColorTerm::fBold << "---Initial Startup---" << ColorTerm::fNormal << "\n");

  // Radar center coordinates
  const LLH center        = r->getRadarLocation();
  const AngleDegs cLat    = center.getLatitudeDeg();
  const AngleDegs cLon    = center.getLongitudeDeg();
  const LengthKMs cHeight = center.getHeightKM();

  // Link to first incoming radar and moment, we will ignore any others from now on
  LogInfo(
    "Linking this algorithm to radar '" << radarName << "' and typename '" << typeName <<
      "' since first pass we only handle 1\n");
  myTypeName  = typeName;
  myRadarName = radarName;

  // myRadarCenter = center;

  // -------------------------------------------------------------
  // Terrain blockage creation
  auto tp = getPlugin<PluginTerrainBlockage>("terrain");

  // Hardcoding for moment.  This range here affects the terrain algorithm
  // We could make it a param if needed.  Or maybe use the info from the tilt?
  LengthKMs rangeKMs = 500;

  if (tp) {
    // This is allowed to be nullptr
    myTerrainBlockage = tp->getNewTerrainBlockage(r->getLocation(), rangeKMs, radarName);
  }

  // -------------------------------------------------------------
  // Elevation volume creation
  auto volp = getPlugin<PluginVolume>("volume");

  if (volp) {
    myElevationVolume = volp->getNewVolume(myRadarName + "_" + myTypeName);
  }
} // PolarMerge::firstDataSetup

void
PolarMerge::processVolume(const Time& aTime, const std::string& subtypeMarker)
{
  // Do the work of processing the virtual volume.  For our 'start' we're gonna try
  // doing an absolute maximum.  This will expand to be plugins that we could also call
  // directly from fusion I think.

  // -------------------------------------------------------------
  // Find the largest gates/radials in the volume and clone that.
  // FIXME: We could also possibly specify the output gates/radials
  // as well. That might work better.
  //
  auto& tilts = myElevationVolume->getVolume();

  if (tilts.size() < 1) { return; }

  size_t maxGates   = 1;
  size_t maxRadials = 1;

  for (size_t i = 0; i < tilts.size(); ++i) {
    // It should be a radial set.
    if (auto r = std::dynamic_pointer_cast<rapio::RadialSet>(tilts[i])) {
      const size_t numGates   = r->getNumGates();
      const size_t numRadials = r->getNumRadials();
      if (numGates > maxGates) {
        maxGates = numGates;
      }
      if (numRadials > maxRadials) {
        maxRadials = numRadials;
      }
    }
  }
  // if (maxRadials > 360){ maxRadials = 360; }
  // if (maxRadials > 720){ maxRadials = 720; }

  // Create the output based on one of the inputs
  auto base = std::dynamic_pointer_cast<rapio::RadialSet>(tilts[0]);

  myMergedSet = RadialSet::Create(base->getTypeName() + "_2DMax",
      base->getUnits(),
      base->getLocation(),
      aTime, // Could see current time or tilt time
      0.5,   // virtual doesn't matter
      base->getDistanceToFirstGateM(),
      base->getGateWidthKMs() * 1000.0,
      maxRadials,
      maxGates);
  myMergedSet->setSubType(subtypeMarker); // Mark special

  // -------------------------------------------------------------
  // Iterate over the volume, projection from largest to the other
  // tilts
  //
  LogInfo(*myElevationVolume << "\n");

  // Do the algorithm.  Here we're going to do an absolute max as POC

  // FIXME: We should make a gate iterator for this type of stuff
  // Iteratring over griss a radials sets.
  // RadialSetIterator and LatLonGridIterator, etc.
  //

  // Gather the radial set projections for speed
  std::vector<RadialSetProjection *> projectors;

  for (size_t i = 0; i < tilts.size(); ++i) {
    auto pd = tilts[i]->getProjection().get();
    RadialSetProjection * p = static_cast<RadialSetProjection *>(pd);
    if (p != nullptr) {
      projectors.push_back(p);
    }
  }


  auto& outData = myMergedSet->getFloat2DRef();
  auto& az      = myMergedSet->getFloat1DRef("Azimuth");
  auto& beam    = myMergedSet->getFloat1DRef("BeamWidth");
  auto& gw      = myMergedSet->getFloat1DRef("GateWidth");
  const float firstGateKM = myMergedSet->getDistanceToFirstGateM() / 1000.0;
  const size_t radials    = myMergedSet->getNumRadials(); // x
  const size_t gates      = myMergedSet->getNumGates();   // y

  for (size_t r = 0; r < radials; ++r) {
    auto beam1 = beam[r];
    auto az1   = az[r] + (beam1 / 2.0); // center
    auto gw1   = gw[r] / 1000.0;        // gatewidth is by azimuth, you would think it woudl be gate

    float KMOut = firstGateKM;
    for (size_t g = 0; g < gates; ++g) {
      float currentMaxAbs = 0.0;
      bool foundOne       = false;
      bool missingMask    = false;

      for (size_t i = 0; i < projectors.size(); ++i) {
        // -----------------------------------------------
        // All the loop work to get 'here' the meat of
        // what we're doing.  Which makes me want to create
        // an iterator class.  Let's see if the code works
        // first than we can refactor
        int radialNo, gateNo;
        double value;
        if (projectors[i]->getValueAtAzRange(az1, KMOut, value, radialNo, gateNo)) {
          missingMask = true; // Because we 'hit' radar coverage

          if (Constants::isGood(value)) {
            value = std::fabs(value);
            if (value >= currentMaxAbs) { currentMaxAbs = value; }
            foundOne = true;
          }
        }
        // -----------------------------------------------
      }

      if (foundOne) {
        outData[r][g] = currentMaxAbs;
      } else {
        outData[r][g] = missingMask ? Constants::MissingData : Constants::DataUnavailable;
      }

      KMOut += gw1; // move along ray
    }
  }

  std::map<std::string, std::string> myOverride;

  writeOutputProduct(myMergedSet->getTypeName(), myMergedSet, myOverride); // Typename will be replaced by -O filters
} // PolarMerge::processVolume

int
main(int argc, char * argv[])
{
  // Create your algorithm instance and run it.  Isn't this API better?
  PolarMerge alg = PolarMerge();

  // Run this thing standalone.
  alg.executeFromArgs(argc, argv);
}
