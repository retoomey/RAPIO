#pragma once

#include "rTerrainBlockage.h"

namespace rapio
{
/* A class for storage information of blockage ranges for the terrain algorithm,
 * @author Lakshman
 */
class PointBlockageLak : public Data
{
public:

  /** Create a point blockage, a 'bin' along a radial line for terrain blockage values */
  PointBlockageLak() : endKMs(std::numeric_limits<float>::max()){ } // not sure max needed here really

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
class TerrainBlockageLak : public TerrainBlockage
{
public:
  /** Introduce to terrain builder by name */
  static void
  introduceSelf();

  /** For STL use only. */
  TerrainBlockageLak()
  { }

  /** Factory method when creating by name. */
  virtual std::shared_ptr<TerrainBlockage>
  create(const std::string& params,
    const LLH             & radarLocation,
    const LengthKMs       & radarRangeKMs, // Range after this is zero blockage
    const std::string     & radarName,
    LengthKMs             minTerrainKMs,
    AngleDegs             minAngleDegs) override;

  /** This should be a LatLonGrid such that the data are
   *  altitude above mean-sea-level in meters.
   */
  TerrainBlockageLak(std::shared_ptr<LatLonGrid> aDEM,
    const LLH                                    & radarLocation,
    const LengthKMs                              & radarRangeKMs,
    const std::string                            & radarName,
    const LengthKMs                              minTerrainKMs = 0,
    const AngleDegs                              minAngleDegs  = 0.1);

  /** Lak's method requires creating a virtual radialset overlay for
   * integrating the blockage within sub 'pencils' of the azimuth coverage area */
  void
  initialize();

  /** Help function, subclasses return help information. */
  virtual std::string
  getHelpString(const std::string& fkey) override;

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
  virtual void
  calculatePercentBlocked(
    // Constants in 3D space information
    LengthKMs stationHeightKMs, AngleDegs beamWidthDegs,
    // Gate information.  For now do center automatically.
    AngleDegs elevDegs, AngleDegs centerAzDegs, LengthKMs centerRangeKMs,
    // Cumulative beam blockage
    float& cbb,
    // Partial beam blockage
    float& pbb,
    // Bottom beam hit
    bool& hit) override;

protected:

  /** Prune rays. (used by computeBeamBlockage) */
  static void
  pruneRayBlockage(std::vector<PointBlockageLak>& orig);

  /**
   * Given the amount of each pencil ray that has been passed,
   * compute the average amount passed by the entire beam.
   */
  static float
  findAveragePassed(const std::vector<float>& pencil_passed);

  /** Calculated beam blockage lookup.  A vector of vectors...so the question is
   * how much lookup speed do we lose here?  If we made a 2D array with max length
   * we could lookup values a lot faster.  We'll see */
  std::vector<std::vector<PointBlockageLak> > myRays;

private:

  /** Have we computed terrain points, etc.? */
  bool myInitialized;

  /** Compute terrain points */
  void
  computeTerrainPoints(
    const LengthKMs              & radarRangeKMs,
    std::vector<PointBlockageLak>& terrainPoints);

  /** Computer beam blockage using the finished computed terrain points */
  void
  computeBeamBlockage(const std::vector<PointBlockageLak>& terrainPoints);
};
}
