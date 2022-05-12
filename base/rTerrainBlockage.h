#pragma once

#include "rData.h"
#include "rUtility.h"

#include "rLatLonGrid.h"
#include "rConstants.h"

#include <boost/multi_array.hpp>

namespace rapio
{
/* A class for storage information of blockage ranges for the terrain algorithm,
 * @author Lakshman, Toomey
 */
class PointBlockage : public Data
{
public:

  /** Create a point blockage, a 'bin' along a radial line for terrain blockage values */
  PointBlockage() : endKMs(INT_MAX){ } // not sure max needed here really

  /**  Does a ray fall into our bin?.  Used during terrain lookup creation */
  bool
  contains(size_t ray_no) const
  {
    return ( ray_no >= minRayNumber && ray_no <= maxRayNumber );
  }

  /** Set the azimuthal spread.  Feel like would be quicker to do directly and skip the
   * function calling.  I'm having to direct pass the boost reference which is less clean */
  void
  setAzimuthalSpread(const boost::multi_array<float, 2>& azimuths, int x, int y);

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
 * Computes the amount by which each radar bin is blocked
 * by terrain.  Interestingly, this is really another kind of 'lookup'
 * for a Radar, so merging that idea with RadialSetLookup might be a good
 * idea at some point.
 *
 * Differences to MRMS version:
 *   --the MIN_HT_ABOUT_TERRAIN is current hardcoded.  It's assumed meters
 *
 * @author Lakshman, Toomey
 */
class TerrainBlockage : public Utility
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
    const LengthKMs                           & radarRangeKMs, // max range radar
    const std::string                         & radarName);

  /** Returns the percent of the beam that was blocked
   *  (to nearest whole integer) The return value is
   *  between 0 and 100 (both numbers inclusive) with 100
   *  indicating a fully blocked beam (no radar return)
   */
  unsigned char
  computePercentBlocked(
    const AngleDegs& beamWidthDegs,
    const AngleDegs& beamElevationDegs,
    const AngleDegs& binAzimuthDegs,
    const LengthKMs& binRangeKMs) const;

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

  /** Computes and returns the height above the terrain
   *  given the elev, az and range from the radar. */
  LengthKMs
  getHeightAboveTerrainKM(const AngleDegs elevDegs,
    const AngleDegs                       azDegs,
    const LengthKMs                       rnKMs) const;

protected:

  /** Store terrain Lat Lon Grid for radar.  The data is heights */
  std::shared_ptr<LatLonGrid> myDEM;

  /** Store LatLonGrid projection for our DEM's height layer */
  std::shared_ptr<LatLonGridProjection> myDEMLookup;

  /** Stores center location of the radar used */
  LLH myRadarLocation;

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

  /** Prune rays. (used by computeBeamBlockage) */
  void
  pruneRayBlockage(std::vector<PointBlockage>& orig);

  /**
   * Given the amount of each pencil ray that has been passed,
   * compute the average amount passed by the entire beam.
   */
  float
  findAveragePassed(const std::vector<float>& pencil_passed) const;
};
}
