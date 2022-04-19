#pragma once

#include <rUtility.h>

#include <rArray.h>

#include <string>
#include <memory>

#include <proj.h>

namespace rapio {
class LatLonGrid;

/** Create a generic projection API wrapper
 *
 * @author Robert Toomey
 */
class Project : public Utility {
public:

  /** Initialize the projection system for this object */
  virtual bool
  initialize();

  // ----------------------------------------------------------------------------
  // General projection methods polar/lat lon
  // @author John Krause, Lak and others for the logic.
  //

  /** Toomey: Implement radar attenuation approximation from
   * Radar Equations for Modern Radar, p. 232.  This is accurate up to
   * 0.4% at about 1000KM */
  static LengthKMs
  attenuationHeightKMs(
    LengthKMs stationHeightKMs,
    LengthKMs rangeKMs,
    AngleDegs elevDegs);

  /** Project from a lat lon to azimuth range based on earth surface */
  static void
  LatLonToAzRange(const AngleDegs &cLat, const AngleDegs &cLon,
    const AngleDegs &tLat, const AngleDegs &tLon, AngleDegs &azDegs, float &rangeMeters);

  /** Project from a amimuth/range to a latitude longitude */
  static void
  BeamPath_AzRangeToLatLon(
    const AngleDegs &stationLatDegs, // !< Station Latitude in degrees
    const AngleDegs &stationLonDegs, // !< Station Long in degrees

    const AngleDegs &azimuthDeg, // !< Target Azimuth in degrees
    const LengthKMs &rangeKMs,   // !< Target Range in kilometers
    const AngleDegs &elevDegs,   // !< Elevation angle in degrees

    LengthKMs       &heightKMs, // !< Height in kilometers
    AngleDegs       &latDegs,   // !< Target Latitude in degrees
    AngleDegs       &lonDegs);  // !< Target Longitude in degree

  /** Calculate observed range, azimuth and elevation */
  static void
  BeamPath_LLHtoAzRangeElev(
    const AngleDegs & targetLatDegs, // !< Target Latitude in degrees
    const AngleDegs & targetLonDegs, // !< Target Longitude in degrees
    const LengthKMs & targetHeightM, // !< Target ht above MSL in meters

    const AngleDegs & stationLatDegs, // !< Station Latitude in degrees
    const AngleDegs & stationLonDegs, // !< Station Long in degrees
    const LengthKMs &stationHeightM,  // Station ht above MSL in meters

    AngleDegs       & elevAngleDegs, // !< Elevation angle in degrees
    AngleDegs       & azimuthDegs,   // !< Target *Azimuth in degrees
    float           & rangeM         // !< Target *Range in meters
  );

  /** Cached version.  Calculate observed range, azimuth and elevation */
  static void
  Cached_BeamPath_LLHtoAzRangeElev(
    const AngleDegs & targetLatDegs, // !< Target Latitude in degrees
    const AngleDegs & targetLonDegs, // !< Target Longitude in degrees
    const LengthKMs & targetHeightM, // !< Target ht above MSL in meters

    const AngleDegs & stationLatDegs, // !< Station Latitude in degrees
    const AngleDegs & stationLonDegs, // !< Station Long in degrees
    const LengthKMs & stationHeightM, // Station ht above MSL in meters
    //
    const double    sinGcdIR, // Cached sin/cos from regular version, this speeds stuff up
    const double    cosGcdIR,

    AngleDegs       & elevAngleDegs, // !< Elevation angle in degrees
    AngleDegs       & azimuthDegs,   // !< Target *Azimuth in degrees
    float           & rangeM         // !< Target *Range in meters
  );

  /** For a station lat lon to target, calculate the sin/cos values for attentuation */
  static void
  stationLatLonToTarget(
    const AngleDegs & targetLatDegs,
    const AngleDegs & targetLonDegs,

    const AngleDegs & stationLatDegs,
    const AngleDegs & stationLonDegs,

    double          & sinGcdIR,
    double          & cosGcdIR);

  /** Using exact same math as the BeamPath_toAzRangeElev, we can calculate the
   * range and height for a different elevation angle */
  static void
  BeamPath_LLHtoAttenuationRange(
    const AngleDegs & targetLatDegs,
    const AngleDegs & targetLonDegs,
    // const LengthKMs & targetHeightKMs, output now

    const AngleDegs & stationLatDegs,
    const AngleDegs & stationLonDegs,
    const LengthKMs & stationHeightKMs,

    const AngleDegs & elevAngleDegs, // need angle now
    //  AngleDegs       & azimuthDegs,  Doesn't matter which direction

    LengthKMs & targetHeightKMs,
    LengthKMs & rangeKMs);

  /** Create Lat Lon Grid marching information from a center and delta degree */
  static void
  createLatLonGrid(const float centerLatDegs, const float centerLonDegrees,
    const float degreeOut, const size_t numRows, const size_t numCols,
    float& topDegs, float& leftDegs, float& deltaLatDegs, float& deltaLonDegs);

  static void
  Cached_BeamPath_LLHtoAttenuationRange(

    const LengthKMs & stationHeightKMs, // need to shift up/down based on station height

    const double    sinGcdIR, // Cache the sin/cos from regular version
    const double    cosGcdIR,

    const AngleDegs & elevAngleDegs, // need angle now
    //  AngleDegs       & azimuthDegs,  Uniform all directions

    LengthKMs & targetHeightKMs,
    LengthKMs & rangeKMs);

  // ----------------------------------------------------------------------------

  /** Get the X/Y kilometer coordinate origin for the raster grid center */
  virtual bool
  getXYCenter(double& centerXKm, double& centerYKm);

  /** Get Lat Lon from given X/Y kilometer coordinates */
  virtual bool
  xyToLatLon(double& x, double&y, double &lat, double&lon);

  /** Get X/Y from given Lat Lon coordinates */
  virtual bool
  LatLonToXY(double& lat, double&lon, double &x, double&y);

  /** Create a lookup grid that has an index for a WDSSII lat lon grid.
   *  I'm suspecting we can do this generically
   * The basic technique is find the X/Y coordinates of the projected center,
   * then kilometer 'march' out from the center
   * Projection is from these X/Y kilometer coordinates to Lat/Lon...
   * 1. F(X/Y) --> F(Lat,Lon)
   * Then using the input lat lon information, we find the index into it
   * 2. F(Lat, Lon) --> F(I,J)
   */
  virtual int
  createLookup(
    // Output information
    int desired_rows, // rows of output projection
    int desired_cols, // cols of output projection
    int mCell,        // Kilometers per cell (sample rate)

    // Input lat lon grid information for projecting lat/lon to index
    // Should we pass the lat lon grid?
    float topleftlat,
    float topleftlon,
    float numlat,
    float numlon,
    float latres,
    float lonres);

  /** Project array to lat lon grid.  This isn't super generic */
  virtual void
  toLatLonGrid(std::shared_ptr<Array<float, 2> > in,
    std::shared_ptr<LatLonGrid> out){ };
};

/* The only subclass for now, projects using the
 * Proj library.  We could have Project be a factory.
 *
 * @author Robert Toomey
 */
class ProjLibProject : public Project
{
public:

  /** Send needed information to the class for initialization */
  ProjLibProject(const std::string& src, const std::string& dst);

  ~ProjLibProject()
  {
    if (myP != nullptr) {
      proj_destroy(myP);
    }
  }

  /** Initialize the projection system */
  virtual bool
  initialize() override;

  /** Get the X/Y kilometer coordinate origin for the raster grid center */
  virtual bool
  getXYCenter(double& centerXKm, double& centerYKm) override;

  /** Get Lat Lon from given X/Y kilometer coordinates */
  virtual bool
  xyToLatLon(double& x, double&y, double &lat, double&lon) override;

  /** Get X/Y from given Lat Lon coordinates */
  virtual bool
  LatLonToXY(double& lat, double&lon, double &x, double&y) override;

  /** Project array to lat lon grid primary.  This isn't super generic. */
  virtual void
  toLatLonGrid(std::shared_ptr<Array<float, 2> > in,
    std::shared_ptr<LatLonGrid> out) override;
private:

  /** Source projection string in Proj library language */
  std::string mySrc;

  /** Destination projection string in Proj library language */
  std::string myDst;

  /** Proj6 object */
  PJ * myP;
};
}
