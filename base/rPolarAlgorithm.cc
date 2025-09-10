#include "rPolarAlgorithm.h"
#include "rColorTerm.h"
#include "rPluginVolume.h"
#include "rPluginTerrainBlockage.h"
#include "rRadialSetIterator.h"
#include "rRadialSetProjection.h"

#include <iostream>

using namespace rapio;

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
  o.optional("upto", "1000",
    "Over this value in elevation angle degrees is ignored.  So upto=3.5 for RadialSets means only include tilts of 3.5 degrees or less.\n");

  return RAPIOAlgorithm::initializeOptions(o);
}

void
PolarAlgorithm::finalizeOptions(RAPIOOptions& o)
{
  myUptoDegs = o.getFloat("upto");
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

void
PolarAlgorithm::processVolume(const Time& useTime, float useElevDegs, const std::string& useSubtype)
{
  LogInfo("Base Algorithm does nothing\n");
} // PolarAlgorithm::processVolume

int
main(int argc, char * argv[])
{
  // Create your algorithm instance and run it.  Isn't this API better?
  PolarAlgorithm alg = PolarAlgorithm();

  // Run this thing standalone.
  alg.executeFromArgs(argc, argv);
}
