#include "rPolarAlgorithm.h"
#include "rColorTerm.h"
#include "rPluginVolume.h"
#include "rPluginTerrainBlockage.h"
#include "rRadialSetIterator.h"
#include "rRadialSetProjection.h"

#include <iostream>

using namespace rapio;

void
PolarAlgorithm::ElevationVolumeCallback::handleBeginLoop(RadialSetIterator * i, const RadialSet& radial)
{
  // We need to project from ground to tilt for each radial set
  auto const numgates = radial.getNumGates();
  auto const gwKMs    = radial.getGateWidthKMs();
  auto const startKMs = (radial.getDistanceToFirstGateM() / 1000.0) + (gwKMs * 0.5);

  auto& pc = getPointerCache();

  myRanges.resize(pc.size());
  for (size_t i = 0; i < pc.size(); ++i) {
    RadialSetPointerCache * rc = static_cast<RadialSetPointerCache *>(pc[i]);
    auto * rs  = static_cast<RadialSet *>(rc->dt);
    auto Tb    = rs->getElevationDegs();
    auto atKMs = startKMs;
    myRanges[i].resize(numgates);
    for (size_t g = 0; g < numgates; ++g) {
      auto atRangeKM = Project::groundToSlantRangeKMs(atKMs, Tb);
      myRanges[i][g] = atRangeKM;
      atKMs += gwKMs;
    }
  }
}

void
PolarAlgorithm::initializePlugins()
{
  // We'll use virtual volumes and terrain ability
  PluginVolume::declare(this, "volume");
  PluginTerrainBlockage::declare(this, "terrain");
  return RAPIOAlgorithm::initializePlugins();
}

void
PolarAlgorithm::initializeOptions(RAPIOOptions& o)
{
  // All polar volume algorithms have volume ability
  // and terrain ability (to use on polar data)
  o.setDefaultValue("terrain", "lak");

  // We'll have default abilities for 'clipping' the volume
  // FIXME: Could these be automatic options for volume plugin?
  // I kinda like that idea, maybe later
  o.optional("topDegs", "1000",
    "Elevation angle top cutoff degrees. Tilts outside are ignored.\n");
  o.optional("bottomDegs", "-1000",
    "Elevation angle bottom cutoff degrees. Tilts outside are ignored.\n");
  o.optional("gates", "-1",
    "Elevation output gates.  -1 uses max gates of tilts in current volume.\n");
  o.optional("azimuths", "-1",
    "Elevation output azimuths.  -1 uses max azimuths of tilts in current volume.\n");

  return RAPIOAlgorithm::initializeOptions(o);
}

void
PolarAlgorithm::finalizeOptions(RAPIOOptions& o)
{
  myTopDegs     = o.getFloat("topDegs");
  myBottomDegs  = o.getFloat("bottomDegs");
  myNumGates    = o.getInteger("gates");
  myNumAzimuths = o.getInteger("azimuths");
  return RAPIOAlgorithm::finalizeOptions(o);
}

void
PolarAlgorithm::processNewData(rapio::RAPIOData& d)
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
PolarAlgorithm::processRadialSet(std::shared_ptr<RadialSet> r)
{
  if (r == nullptr) { return false; }

  // Clip based on RadialSet angle
  auto elevDegs = r->getElevationDegs();

  if ((elevDegs > myTopDegs) || (elevDegs < myBottomDegs)) {
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
} // PolarAlgorithm::processRadialSet

void
PolarAlgorithm::firstDataSetup(std::shared_ptr<RadialSet> r, const std::string& radarName,
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
} // PolarAlgorithm::firstDataSetup

std::shared_ptr<RadialSet>
PolarAlgorithm::createOutputRadialSet(
  const Time& useTime, float useElevDegs, const std::string& useTypename, const std::string& useSubtype)
{
  if (myElevationVolume == nullptr) { return nullptr; }

  auto& tilts = myElevationVolume->getVolume();

  if (tilts.size() < 1) { return nullptr; }

  // ----------------------------------
  // Dynamic size based on the max size of tilt in a volume.
  // FIXME: A max dimensions method for ElevationVolume/Volume?
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
  // Replace with setting if param passed
  if (myNumGates > -1) { maxGates = myNumGates; }
  if (myNumAzimuths > -1) { maxRadials = myNumAzimuths; }

  // ----------------------------------
  // if (maxRadials > 360){ maxRadials = 360; }
  // if (maxRadials > 720){ maxRadials = 720; }

  // Create the output based on one of the inputs
  auto base = std::dynamic_pointer_cast<rapio::RadialSet>(tilts[0]);

  // FIXME: Humm a polar might want to change units I think.
  // Some other settings we might need access as well
  auto set = RadialSet::Create(useTypename,
      base->getUnits(),
      base->getLocation(),
      useTime, // Could see current time or tilt time
      useElevDegs,
      base->getDistanceToFirstGateM(),
      base->getGateWidthKMs() * 1000.0,
      maxRadials,
      maxGates);

  set->setSubType(useSubtype); // Mark special
  // Debating forcing RadarName in Create, but make sure we
  // have one since many algs require this field.
  set->setRadarName(base->getRadarName());
  // ---------------------------------------------------

  return set;
} // PolarAlgorithm::createOutputRadialSet

void
PolarAlgorithm::processVolume(const Time& useTime, float useElevDegs, const std::string& useSubtype)
{
  LogInfo("Base Algorithm does nothing\n");
} // PolarAlgorithm::processVolume
