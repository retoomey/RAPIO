#pragma once

#include "rTerrainBlockage.h"
#include "rData.h"
#include "rUtility.h"
#include "rFactory.h"

#include "rLatLonGrid.h"
#include "rConstants.h"

#include <limits>

namespace rapio
{
/** Attempt to make a faster polar terrain blockage
 * @author Robert Toomey
 */
class TerrainBlockage2me : public TerrainBlockage
{
public:
  /** Introduce to terrain builder by name */
  static void
  introduceSelf();

  /** Factory method when creating by name. */
  virtual std::shared_ptr<TerrainBlockage>
  create(const std::string& params,
    const LLH             & radarLocation,
    const LengthKMs       & radarRangeKMs, // Range after this is zero blockage
    const std::string     & radarName,
    LengthKMs             minTerrainKMs,
    AngleDegs             minAngleDegs) override;

  /** For STL use only. */
  TerrainBlockage2me()
  { }

  /** Create TerrainBlockage given a DEM and radar information
   */
  TerrainBlockage2me(std::shared_ptr<LatLonGrid> aDEM,
    const LLH                                    & radarLocation,
    const LengthKMs                              & radarRangeKMs,
    const std::string                            & radarName,
    const LengthKMs                              minTerrainKMs,
    const AngleDegs                              minAngleDegs);

  /** Help function, subclasses return help information. */
  virtual std::string
  getHelpString(const std::string& fkey) override;

  /** First attempt at general per gate method */
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
};
}
