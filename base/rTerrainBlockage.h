#pragma once

#include "rData.h"
#include "rUtility.h"
#include "rFactory.h"
#include "rLatLonGrid.h"

namespace rapio
{
/** Base class for creating a terrain blockage algorithm for a particular radar.
 * Also base factory for creating a terrain blockage by registered name.
 *
 * @author Robert Toomey
 */
class TerrainBlockage : public Utility
{
public:
  /** For STL use only. */
  TerrainBlockage()
  { }

  // --------------------------------------------------------
  // Factory methods for doing things by name.  Usually if you
  // want to support command line choosing of a TerrainBlockage
  // algorithm.

  /** Use this to introduce default built-in RAPIO TerrainBlockage subclasses.
   * Note: You don't have to use this ability, it's not called by default algorithm.
   * To use it, call TerrainBlockage::introduceSelf()
   * To override or add another, call TerrainBlockage::introduce(myterrainkey, newterrain)
   */
  static void
  introduceSelf();

  /** Introduce a new TerrainBlockage by name */
  static void
  introduce(const std::string        & key,
    std::shared_ptr<TerrainBlockage> factory);

  /** Attempt to create TerrainBlockage by name.  This looks up any registered
   * classes and attempts to call the virtual create method on it.
   * FIXME: could key pair parameters for generalization, for now constant */
  static std::shared_ptr<TerrainBlockage>
  createTerrainBlockage(const std::string & key,
    // Could make general args and improve interface here
    std::shared_ptr<LatLonGrid>           aDEM,
    const LLH                             & radarLocation,
    const LengthKMs                       & radarRangeKMs, // Range after this is zero blockage
    const std::string                     & radarName,
    LengthKMs                             minTerrainKMs = 0,    // Bottom beam touches this we're blocked
    AngleDegs                             minAngle      = 0.1); // Below this, no blockage occurs

protected:

  /** Called on subclasses by the TerrainBlockage to create/setup the TerrainBlockage.
   * To use by name, you would override this method and return a new instance of your
   * TerrainBlockage class. */
  virtual std::shared_ptr<TerrainBlockage>
  create(std::shared_ptr<LatLonGrid> aDEM,
    const LLH                        & radarLocation,
    const LengthKMs                  & radarRangeKMs, // Range after this is zero blockage
    const std::string                & radarName,
    LengthKMs                        minTerrainKMs,
    AngleDegs                        minAngleDegs)
  {
    return nullptr;
  }

public:
  // --------------------------------------------------------

  /** Create TerrainBlockage given a DEM and radar information */
  TerrainBlockage(std::shared_ptr<LatLonGrid> aDEM,
    const LLH                                 & radarLocation,
    const LengthKMs                           & radarRangeKMs, // Range after this is zero blockage
    const std::string                         & radarName,
    LengthKMs                                 minTerrainKMs,
    AngleDegs                                 minAngleDegs);

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
  getPowerDensity(float dist)
  {
    // FIXME: what units?
    float x = M_PI * 1.27 * dist;

    if (x < 0.01) {
      x = 0.01; // avoid divide-by-zero
    }
    float wt = (1 - exp(-x * x / 8.5)) / (x * x);

    wt = wt * wt;
    return (wt);
  }

  // --------------------------------------------------
  // Worker functions to actually calculate blockages
  // --------------------------------------------------

  /** Calculate terrain blockage percentage for a given RadialSet for each gate, and
   * add a 2D array to the RadialSet called TerrainPercent to store this value */
  void
  calculateTerrainPerGate(std::shared_ptr<RadialSet> r);

  /** Calculate percentage blocked information for a point */
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
    bool& hit) = 0;

protected:

  /** Store terrain LatLonGrid for radar.  The data is height in meters */
  std::shared_ptr<LatLonGrid> myDEM;

  /** Store LatLonGrid projection for our DEM's height layer */
  std::shared_ptr<LatLonGridProjection> myDEMLookup;

  /** Stores center location of the radar used */
  LLH myRadarLocation;

  /** The minimum bottem beam terrain height before 100% blockage */
  LengthKMs myMinTerrainKMs;

  /** The minimum angle before applying terrain blocking */
  LengthKMs myMinAngleDegs;

  /** The max range before terrain has zero effect */
  LengthKMs myMaxRangeKMs;
};
}
