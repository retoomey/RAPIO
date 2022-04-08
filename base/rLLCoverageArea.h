#pragma once

#include <rData.h>
#include <rConstants.h>

namespace rapio {
/** Represents a sector of Latitude Longitude that is broken up into a grid,
 * with the ability to mark a subset of that area.
 *
 * FIXME: I use grids other places so maybe combine/simplify this.  The image marching
 * and lat lon grids, etc. might just have a shared object.  There is a LLCoverage
 * in DataProjection that's used slightly differently and a bit messier.
 *
 * @author Robert Toomey
 **/
class LLCoverageArea : public Data
{
public:

  /** Create a blank coverage grid */
  LLCoverageArea(){ };

  /** Create a coverage grid */
  LLCoverageArea(AngleDegs north, AngleDegs west, AngleDegs south, AngleDegs east, AngleDegs southDelta,
    AngleDegs eastSpacing,
    size_t aNumX, size_t aNumY) : nwLat(north), nwLon(west), seLat(south), seLon(east),
    latSpacing(southDelta), lonSpacing(eastSpacing), startX(0), startY(0), numX(aNumX), numY(aNumY){ }

  /** Inset our grid to a given radar center and range */
  LLCoverageArea
  insetRadarRange(
    // Input Radar center and max range for box clipping.
    AngleDegs cLat, AngleDegs cLon, LengthKMs rangeKMs
  ) const;

  // FIXME: 'Maybe' get/set

  // Angles of the four sides
  AngleDegs nwLat; // !< North side of box
  AngleDegs nwLon; // !< West side of box
  AngleDegs seLat; // !< South side of box
  AngleDegs seLon; // !< East side of box

  // Marching information
  AngleDegs latSpacing; // !< Spacing per cell east to west
  AngleDegs lonSpacing; // !< Spacing per cell north to south

  // Index information
  size_t startX; // !< Starting grid X or longitude cell
  size_t startY; // !< Starting grid Y or latitude cell
  size_t numX;   // !< Total number of longitude cells
  size_t numY;   // !< Total number of latitude cells
};

std::ostream&
operator << (std::ostream&,
  const LLCoverageArea&);
}
