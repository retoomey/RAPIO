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
  o.optional("upto", "1000",
    "Over this value in elevation angle degs is ignored.  So upto=3.5 for RadialSets means only include tilts of 3.5 or less.\n");
}

void
PolarMerge::processOptions(RAPIOOptions& o)
{
  myUptoDegs = o.getFloat("upto");
}

void
PolarMerge::processNewData(rapio::RAPIOData& d)
{
  // Do we have a radial set then process it
  auto r = d.datatype<rapio::RadialSet>();

  if (r != nullptr) {
    const bool addedTilt = processRadialSet(r);


    if (addedTilt) {
      // The time, elev angle, and direct subtype for output folder
      // Subtype attribute will be the elev angle, but we'll write "at"
      const std::string useSubtype = "at" + r->getSubType();
      const float useElevDegs      = r->getElevationDegs();
      const Time useTime = r->getTime(); // Triggering output tilt, use time of tilt

      processVolume(useTime, useElevDegs, useSubtype); // Since just added, use time of incoming
    }
  }
}

bool
PolarMerge::processRadialSet(std::shared_ptr<RadialSet> r)
{
  if (r->getElevationDegs() > myUptoDegs) {
    return false;
  }

  LogInfo(ColorTerm::green() << ColorTerm::bold() << "---RadialSet---" << ColorTerm::reset() << "\n");
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
PolarMerge::processVolume(const Time& useTime, float useElevDegs, const std::string& useSubtype)
{
  /** New way using the callback iterator API.  This will allow us to hide
   * the 'normal' ways of iterating over DataTypes.  In particular,
   * the LatLonGrid is complicated. Typically one marches over lat/lon and
   * has to do angle calculations, etc.  We can hide all that from users. */
  class myRadialSetCallback : public RadialSetCallback {
public:

    /** Add a projection for a RadialSet.  We'll project into
     * each tilt to get the value there */
    void
    addProjection(RadialSetProjection * p)
    {
      projectors.push_back(p);
    }

    /** For each gate, we've gonna take the absolute max of the other gates
     * in the volume vertically */
    virtual void
    handleGate(RadialSetIterator * it)
    {
      // We want the current center azimuth/range of our RadialSet gate,
      // this will be used to project into the other RadialSets.
      const auto atAzDegs  = it->getCenterAzimuthDegs();
      const auto atRangeKM = it->getCenterRangeMeters() / 1000.0; // FIXME: units?

      float currentMaxAbs = 0.0;
      bool foundOne = false;
      bool missingMask = false;
      int radialNo, gateNo;
      double value;

      // For each RadialSet in the volume...
      for (size_t i = 0; i < projectors.size(); ++i) {
        // ...see if we hit it...
        if (projectors[i]->getValueAtAzRange(atAzDegs, atRangeKM, value, radialNo, gateNo)) {
          missingMask = true; // Because we 'hit' radar coverage

          // ...and if a good value, keep the maximum
          if (Constants::isGood(value)) {
            value = std::fabs(value);
            if (value >= currentMaxAbs) { currentMaxAbs = value; }
            foundOne = true;
          }
        }

        // Maybe a group method for this?  This is probably a common thing
        // it->setValueMask(foundOne, missingMask, value);

        if (foundOne) {
          it->setValue(currentMaxAbs);
          //  outData[r][g] = currentMaxAbs;
        } else {
          it->setValue(missingMask ? Constants::MissingData : Constants::DataUnavailable);
          // outData[r][g] = missingMask ? Constants::MissingData : Constants::DataUnavailable;
        }
      }
    } // handleGate

private:
    // Gather the radial set projections for speed
    std::vector<RadialSetProjection *> projectors;
  };

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
      useTime, // Could see current time or tilt time
      useElevDegs,
      base->getDistanceToFirstGateM(),
      base->getGateWidthKMs() * 1000.0,
      maxRadials,
      maxGates);
  myMergedSet->setSubType(useSubtype); // Mark special
  // Debating forcing RadarName in Create, but make sure we
  // have one since many algs require this field.
  myMergedSet->setRadarName(base->getRadarName());

  // -------------------------------------------------------------
  // Iterate over the volume, projection from largest to the other
  // tilts
  //
  LogInfo(*myElevationVolume << "\n");

  // Do the algorithm.  Here we're going to do an absolute max as POC

  myRadialSetCallback myCallback;

  // Gather the radial set projections for speed
  for (size_t i = 0; i < tilts.size(); ++i) {
    auto pd = tilts[i]->getProjection().get();
    RadialSetProjection * p = static_cast<RadialSetProjection *>(pd);
    if (p != nullptr) {
      myCallback.addProjection(p);
    }
  }

  RadialSetIterator iter(*myMergedSet);

  iter.iterateRadialGates(myCallback);

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
