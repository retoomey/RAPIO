#pragma once

#include "rData.h"
#include "rUtility.h"

#include "rLatLonGrid.h"
#include "rConstants.h"

#include <limits>

namespace rapio
{
/** Base class for creating a terrain blockage algorithm for a particular radar.
 * @author Robert Toomey
 */
class TerrainBlockageBase : public Utility
{
public:
  /** For STL use only. */
  TerrainBlockageBase()
  { }

  /** Create TerrainBlockage given a DEM and radar information */
  TerrainBlockageBase(std::shared_ptr<LatLonGrid> aDEM,
    const LLH                                     & radarLocation,
    const LengthKMs                               & radarRangeKMs, // Range after this is zero blockage
    const std::string                             & radarName,
    LengthKMs                                     minTerrain = 0);

  // --------------------------------------------------
  // Accessing DEM heights
  // --------------------------------------------------

  /** Get the height from the DEM nearest lat/lon location */
  LengthKMs
  getTerrainHeightKMs(const AngleDegs latDegs, const AngleDegs lonDegs);

  /** Computes and returns the height above the terrain
   *  given the elev, az and range from the radar. */
  LengthKMs
  getHeightKM(const AngleDegs elevDegs,
    const AngleDegs           azDegs,
    const LengthKMs           rnKMs,
    AngleDegs                 & outLatDegs,
    AngleDegs                 & outLonDegs) const;

  /** Computes and returns the height above the terrain
   *  given the elev, az and range from the radar. */
  LengthKMs
  getHeightAboveTerrainKM(const AngleDegs elevDegs,
    const AngleDegs                       azDegs,
    const LengthKMs                       rnKMs) const;

  // --------------------------------------------------
  // Meteorology functions.  Maybe separate to a dedicated class
  // --------------------------------------------------

  /** Doppler Radar and Weather Observations, 2nd Edition.
   *
   * Calculate  eq. 3.2a of Doviak-Zrnic
   * gives the power density function
   *   theta -- angular distance from the beam axis = dist * beamwidth
   *   sin(theta) is approx theta = dist*beamwidth
   *  lambda (eq.3.2b) = beamwidth*D / 1.27
   *  thus, Dsin(theta)/lambda = D * dist * beamwidth / (D*beamwidth/1.27)
   *                           = 1.27 * dist
   *  and the weight is ( J_2(pi*1.27*dist)/(pi*1.27*dist)^2 )^2
   *     dist is between -1/2 and 1/2, so abs(pi*1.27*dist) < 2
   *  in this range, J_2(x) is approximately 1 - exp(-x^2/8.5)
   *
   */
  inline static float
  getPowerDensity(float dist); // FIXME: what units?

  // --------------------------------------------------
  // Worker functions to actualy calculate blockages
  // --------------------------------------------------

  /** Calculate terrain blockage percentage for a given RadialSet for each gate, and
   * add a 2D array to the RadialSet called TerrainPercent to store this value */
  void
  calculateTerrainPerGate(std::shared_ptr<RadialSet> r);

  /** First attempt at general per gate method */
  virtual void
  calculateGate(
    // Constants in 3D space information
    LengthKMs stationHeightKMs, AngleDegs beamWidthDegs,
    // Gate information.  For now do center automatically.
    AngleDegs elevDegs, AngleDegs centerAzDegs, LengthKMs centerRangeKMs,
    // Largest PBB
    float& greatestPercentage,
    // Final output percentage for gate
    float& v) = 0;

protected:

  /** Store terrain LatLonGrid for radar.  The data is height in meters */
  std::shared_ptr<LatLonGrid> myDEM;

  /** Store LatLonGrid projection for our DEM's height layer */
  std::shared_ptr<LatLonGridProjection> myDEMLookup;

  /** Stores center location of the radar used */
  LLH myRadarLocation;

  /** The minimum bottem beam terrain height before 100% blockage */
  LengthKMs myMinTerrainKMs;
};

/** Attempt to make a faster polar terrain blockage
 * @author Robert Toomey
 */
class TerrainBlockage2 : public TerrainBlockageBase
{
public:
  /** For STL use only. */
  TerrainBlockage2()
  { }

  /** This should be a LatLonGrid such that the data are
   *  altitude above mean-sea-level in meters.
   */
  TerrainBlockage2(std::shared_ptr<LatLonGrid> aDEM,
    const LLH                                  & radarLocation,
    const LengthKMs                            & radarRangeKMs,
    const std::string                          & radarName);

  /** First attempt at general per gate method */
  virtual void
  calculateGate(
    // Constants in 3D space information
    LengthKMs stationHeightKMs, AngleDegs beamWidthDegs,
    // Gate information.  For now do center automatically.
    AngleDegs elevDegs, AngleDegs centerAzDegs, LengthKMs centerRangeKMs,
    // Largest PBB
    float& greatestPercentage,
    // Final output percentage for gate
    float& v);
};

/* A class for storage information of blockage ranges for the terrain algorithm,
 * @author Lakshman
 */
class PointBlockage : public Data
{
public:

  /** Create a point blockage, a 'bin' along a radial line for terrain blockage values */
  PointBlockage() : endKMs(std::numeric_limits<float>::max()){ } // not sure max needed here really

  /**  Does a ray fall into our bin?.  Used during terrain lookup creation */
  bool
  contains(size_t ray_no) const
  {
    return ( ray_no >= minRayNumber && ray_no <= maxRayNumber );
  }

  /** Set the azimuthal spread.  Feel like would be quicker to do directly and skip the
   * function calling.  I'm having to direct pass the boost reference which is less clean */
  static void
  setAzimuthalSpread(const boost::multi_array<float, 2>& azimuths, int x, int y, size_t& min, size_t& max);

  /** Calculate ray number from angle */
  static size_t
  getRayNumber(float ang);

  /** The azimuth angle represent */
  AngleDegs azDegs;

  /** The elevation angle represent */
  AngleDegs elevDegs;

  /** Starting range along the ray */
  LengthKMs startKMs;

  /** Ending range along the ray */
  LengthKMs endKMs;

  /** Minimum ray number */
  size_t minRayNumber;

  /** Maximum ray number */
  size_t maxRayNumber;
};

/**
 * Terrain blockage from MRMS
 *
 * @author Lakshman
 */
class TerrainBlockage : public TerrainBlockageBase
{
public:
  /** For STL use only. */
  TerrainBlockage()
  { }

  /** This should be a LatLonGrid such that the data are
   *  altitude above mean-sea-level in meters.
   */
  TerrainBlockage(std::shared_ptr<LatLonGrid> aDEM,
    const LLH                                 & radarLocation,
    const LengthKMs                           & radarRangeKMs,
    const std::string                         & radarName);

  /** First attempt at general per gate method */
  virtual void
  calculateGate(
    // Constants in 3D space information
    LengthKMs stationHeightKMs, AngleDegs beamWidthDegs,
    // Gate information.  For now do center automatically.
    AngleDegs elevDegs, AngleDegs centerAzDegs, LengthKMs centerRangeKMs,
    // Largest PBB
    float& greatestPercentage,
    // Final output percentage for gate
    float& v) override;

  /**
   *
   * Returns 0 for non-blocked beams and 1 for fully blocked ones.
   * The binAzimuth is the *center* azimuth of the radial.
   *
   * See O'Bannon, T., 1997: Using a terrain-based hybrid scan to improve WSR-88D precipitation
   * estimates. Preprints, 28th Conf. on Radar Meteorology, Austin, TX, Amer. Meteor. Soc., 506
   *
   * Tim's hybrid scan technique is also described in some detail in:
   *
   * Fulton, R. A., J. P. Breidenbach, D. J. Seo, D. A. Miller, and T. O'Bannon, 1998:
   * The WSR-88D rainfall algorithm. Wea. Forecasting., 13, 377
   * and
   * Maddox, Robert A., Zhang, Jian, Gourley, Jonathan J., Howard, Kenneth W. 2002:
   * Weather Radar Coverage over the Contiguous United States. Weather and Forecasting:
   * Vol. 17, No. 4, pp. 927
   */
  float
  computeFractionBlocked(const AngleDegs& beamWidthDegs,
    const AngleDegs                     & beamElevationDegs,
    const AngleDegs                     & binAzimuthDegs,
    const LengthKMs                     & binRangeKMs) const;

  /** Prune rays. (used by computeBeamBlockage) */
  static void
  pruneRayBlockage(std::vector<PointBlockage>& orig);

  /**
   * Given the amount of each pencil ray that has been passed,
   * compute the average amount passed by the entire beam.
   */
  static float
  findAveragePassed(const std::vector<float>& pencil_passed);

protected:

  /** Calculated beam blockage lookup.  A vector of vectors...so the question is
   * how much lookup speed do we lose here?  If we made a 2D array with max length
   * we could lookup values a lot faster.  We'll see */
  std::vector<std::vector<PointBlockage> > myRays;

private:

  /** Compute terrain points */
  void
  computeTerrainPoints(
    const LengthKMs           & radarRangeKMs,
    std::vector<PointBlockage>& terrainPoints);

  /** Computer beam blockage using the finished computed terrain points */
  void
  computeBeamBlockage(const std::vector<PointBlockage>& terrainPoints);
};
}
