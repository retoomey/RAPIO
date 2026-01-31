#pragma once

#include <rArray.h>

#include <string>
#include <memory>

#if HAVE_PROJLIB
# include <proj.h>
#endif

namespace rapio {
class LatLonGrid;

/** Create a generic projection API wrapper
 *
 * @author Robert Toomey
 * @ingroup rapio_data
 * @brief Generic GIS projection API wrapper.
 */
class Project {
public:

  /** Initialize the projection system for this object */
  virtual bool
  initialize();

  // ----------------------------------------------------------------------------
  // General projection methods polar/lat lon
  // @author John Krause, Lak and others for the logic.
  //

  /** Project from a ground range up to a slant range along radar.
   * This is a 'quick' approximation.  MRMS used this in reverse for polar
   * algorithms.  Basically cos of the elevation angle shortens the distance */
  static LengthKMs
  groundToSlantRangeKMs(LengthKMs groundRangeKMs, AngleDegs elevDegs);

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
    const AngleDegs &stationLatDegs, ///< Station Latitude in degrees
    const AngleDegs &stationLonDegs, ///< Station Long in degrees

    const AngleDegs &azimuthDeg, ///< Target Azimuth in degrees
    const LengthKMs &rangeKMs,   ///< Target Range in kilometers
    const AngleDegs &elevDegs,   ///< Elevation angle in degrees

    LengthKMs       &heightKMs, ///< Height in kilometers
    AngleDegs       &latDegs,   ///< Target Latitude in degrees
    AngleDegs       &lonDegs);  ///< Target Longitude in degree

  /** Calculate observed range, azimuth and elevation */
  static void
  BeamPath_LLHtoAzRangeElev(
    const AngleDegs & targetLatDegs, ///< Target Latitude in degrees
    const AngleDegs & targetLonDegs, ///< Target Longitude in degrees
    const LengthKMs & targetHeightM, ///< Target ht above MSL in kilometers

    const AngleDegs & stationLatDegs, ///< Station Latitude in degrees
    const AngleDegs & stationLonDegs, ///< Station Long in degrees
    const LengthKMs &stationHeightM,  ///< Station ht above MSL in kilometers

    AngleDegs       & elevAngleDegs, ///< Elevation angle in degrees
    AngleDegs       & azimuthDegs,   ///< Target *Azimuth in degrees
    LengthKMs       & rangeKMs       ///< Target *Range in kilometers
  );

  /** Cached version.  Calculate observed range, azimuth and elevation */
  static void
  Cached_BeamPath_LLHtoAzRangeElev(
    const AngleDegs & targetLatDegs, ///< Target Latitude in degrees
    const AngleDegs & targetLonDegs, ///< Target Longitude in degrees
    const LengthKMs & targetHeightM, ///< Target ht above MSL in kilometers

    const AngleDegs & stationLatDegs, ///< Station Latitude in degrees
    const AngleDegs & stationLonDegs, ///< Station Long in degrees
    const LengthKMs & stationHeightM, ///< Station ht above MSL in kilometers
    //
    const double    sinGcdIR, // Cached sin/cos from regular version, this speeds stuff up
    const double    cosGcdIR,

    AngleDegs       & elevAngleDegs, ///< Elevation angle in degrees
    AngleDegs       & azimuthDegs,   ///< Target *Azimuth in degrees
    LengthKMs       & rangeKMs       ///< Target *Range in kilometers
  );

  /** For a station lat lon to target, calculate the sin/cos values for attentuation.
   * These can be cached and passed to other projection functions. */
  static void
  stationLatLonToTarget(
    const AngleDegs & targetLatDegs, ///< Target latitude degrees
    const AngleDegs & targetLonDegs, ///< Target longitude degrees

    const AngleDegs & stationLatDegs, ///< The radar center latitude
    const AngleDegs & stationLonDegs, ///< The radar center longitude

    double          & sinGcdIR,  ///< sin(gcd/IR) output
    double          & cosGcdIR); ///< cos(gcd/IR) output

  /** Using exact same math as the BeamPath_toAzRangeElev, we can calculate the
   * range and height for a different elevation angle.
   * The caching version is much faster for cube marching, but requires caching
   * trigonometric functions using stationLatLonToTarget and RadialSet caching
   * of elevation angle. This uses 4/3 attenuation of atmosphere */
  static void
  BeamPath_LLHtoAttenuationRange(
    const AngleDegs & targetLatDegs, ///< Target latitude degrees
    const AngleDegs & targetLonDegs, ///< Target longitude degrees

    const AngleDegs & stationLatDegs,   ///< The radar center latitude
    const AngleDegs & stationLonDegs,   ///< The radar center longitude
    const LengthKMs & stationHeightKMs, ///< The radar center height in kilometers

    const AngleDegs & elevAngleDegs, ///< The elevation angle in degrees of radar tilt

    LengthKMs       & targetHeightKMs, ///< Output height perpendicular to earth of beam
    LengthKMs       & rangeKMs);       ///< Output range along the curved elevation beam

  /** Create Lat Lon Grid marching information from a center and delta degree */
  static void
  createLatLonGrid(const float centerLatDegs, const float centerLonDegrees,
    const float degreeOut, const size_t numRows, const size_t numCols,
    float& topDegs, float& leftDegs, float& deltaLatDegs, float& deltaLonDegs);

  /** A caching version of LLHtoAttenuationRange which uses cached values of
   * trigonometric functions for speed.  This uses 4/3 attenuation of atmosphere.
   * Inlining this since volume resolver uses this heavily in realtime for
   * resolving the volume. */
  inline static void
  Cached_BeamPath_LLHtoAttenuationRange(

    const LengthKMs & stationHeightKMs, // need to shift up/down based on station height

    const double    sinGcdIR, ///< sin(gcd/IR) Cache the sin/cos from regular version
    const double    cosGcdIR, ///< cos(gcd/IR) which is from radar center to LL location

    const double    tanElev, ///< Cached tangent of the elevation angle
    const double    cosElev, ///< Cached cosine of the elevation angle

    LengthKMs       & targetHeightKMs, ///< Output height perpendicular to earth of beam
    LengthKMs       & rangeKMs)        ///< Output range along the curved elevation beam
  {
    constexpr double IR = (4. / 3.) * Constants::EarthRadiusM;

    // -----------------------------------------------------------------------------------
    // Verified reverse formula from new elev to height here
    // heightM = (IR/(-sinGcdIR*tan(newElevRad) + cos(great_circle_distance/IR))) - IR;
    // targetHeightKMs = (heightM/1000.0) + stationHeightKMs;
    // -----------------------------------------------------------------------------------
    const double newHeightM = (IR / (-sinGcdIR * tanElev + cosGcdIR)) - IR;

    targetHeightKMs = (newHeightM / 1000.0) + stationHeightKMs;

    rangeKMs = (( sinGcdIR ) * (IR + newHeightM) / cosElev) / 1000.0;
  }

  /** Destination point given distance and bearing from start point */
  static void
  LLBearingDistance(
    const AngleDegs startLatDegs,
    const AngleDegs startLonDegs,
    const AngleDegs bearing,
    const LengthKMs distance,
    AngleDegs       & outLat,
    AngleDegs       & outLon
  );


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
  ProjLibProject(const std::string& src, const std::string& dst = "+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");

  /** Destroy a project lib */
  ~ProjLibProject();

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
  #if HAVE_PROJLIB
  PJ * myP;
  #else
  void * myP;
  #endif
};
}
