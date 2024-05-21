#pragma once

#include <rData.h>
#include <rConstants.h>

#include <vector>

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

  /** Sync anything relying on the values, such as distances */
  void
  sync();

  /** Set values, called by readers */
  void
  set(AngleDegs north, AngleDegs west, AngleDegs south, AngleDegs east, AngleDegs southDelta,
    AngleDegs eastSpacing, size_t aNumX, size_t aNumY);

  /** Lookup a string such as "NMQWD" and return the incr/upto lists used
   * to generate a height list */
  bool
  lookupIncrUpto(const std::string& key, std::vector<int>& incr, std::vector<int>& upto);

  /** Parse the heights from parameter information such as "ne" and "55, -130" */
  bool
  parseHeights(const std::string& label, const std::string& list, std::vector<double>& heightsMs);

  /** Parse a pair of degrees, used for nw, se, and spacing functions */
  bool
  parseDegrees(const std::string& label, const std::string& p, AngleDegs& lat, AngleDegs& lon);

  /** Parse from strings, used to generate from command line options.  This supports
   * the legacy -t -b -s and the combined cleaner -grid option */
  bool
  parse(const std::string& grid, const std::string& t = "", const std::string& b = "", const std::string& s = "");

  /** Inset our grid to a given radar center and range */
  LLCoverageArea
  insetRadarRange(
    // Input Radar center and max range for box clipping.
    AngleDegs cLat, AngleDegs cLon, LengthKMs rangeKMs
  ) const;

  /** Partition our grid into X by Y subgrids.  X is east-west, Y is north-south.
   * Tiles will be numbered from left to right 0 at the northwest corner in row major.
   * For example, for a 3, 2 tiling, there will be 6 total tiles, in the following
   * order (base 1):
   * 1 2 3
   * 4 5 6
   */
  void
  tile(const size_t x, const size_t y, std::vector<LLCoverageArea>& tiles) const;

  inline AngleDegs
  getNWLat() const { return nwLat; } ///<Get north side of box

  inline AngleDegs
  getNWLon() const { return nwLon; } ///<Get west side of box

  inline AngleDegs
  getSELat() const { return seLat; } ///<Get south side of box

  inline AngleDegs
  getSELon() const { return seLon; } ///<Get east side of box

  inline AngleDegs
  getLatSpacing() const { return latSpacing; } ///<Get the spacing per cell east to west

  inline AngleDegs
  getLonSpacing() const { return lonSpacing; } ///<Get the spacing per cell north to south

  inline size_t
  getStartX() const { return startX; } ///<Get the start X

  inline size_t
  getStartY() const { return startY; } ///<Get the start Y

  inline size_t
  getNumX() const { return numX; } ///<Get the total X count

  inline size_t
  getNumY() const { return numY; } ///<Get the total Y count

  inline size_t
  getNumZ() const { return heightsKM.size(); } // <Get the number of optional layers

  inline LengthKMs
  getLatKMPerPixel() const { return latKMPerPixel; } ///< Estimate of KMs per cell in latitude

  inline LengthKMs
  getLonKMPerPixel() const { return lonKMPerPixel; } ///< Estimate of KMS per cell in longitude

  /** Get the list of optional heights */
  std::vector<double>
  getHeightsKM() const { return heightsKM; }

  /** Get unique parse string using a _ format that can be used as part of a unique folder name */
  std::string
  getParseUniqueString() const;

  /** Allow operator << to access our internal fields */
  friend std::ostream&
  operator << (std::ostream&, const LLCoverageArea&);

protected:
  // Angles of the four sides
  AngleDegs nwLat; ///< North side of box
  AngleDegs nwLon; ///< West side of box
  AngleDegs seLat; ///< South side of box
  AngleDegs seLon; ///< East side of box

  // Marching information
  AngleDegs latSpacing; ///< Spacing per cell east to west
  AngleDegs lonSpacing; ///< Spacing per cell north to south

  // Index information
  size_t startX; ///< Starting grid X or longitude cell
  size_t startY; ///< Starting grid Y or latitude cell

  size_t numX; ///< Total number of longitude cells
  size_t numY; ///< Total number of latitude cells

  LengthKMs latKMPerPixel; ///< Estimate of KMs per cell in latitude
  LengthKMs lonKMPerPixel; ///< Estimate of KMS per cell in longitude

  /** An optional collection of heights in meters */
  std::vector<double> heightsKM;

  /** If created by parsing heights, this holds the unique string
   * that was used before parsing */
  std::string myHeightParse;
};

std::ostream&
operator << (std::ostream&,
  const LLCoverageArea&);
}
