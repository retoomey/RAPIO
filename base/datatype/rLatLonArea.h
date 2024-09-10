#pragma once

#include <rDataGrid.h>
#include <rLLH.h>
#include <rTime.h>
#include <rLLCoverageArea.h>

namespace rapio {
/** Store an area of data in Lat Lon Height space of some sort
 *  We have a rLLCoverageArea dealing with parameter grid.
 *  We have a rLLCoverage dealing with projection stuff.
 *  And this deals with a DataType doing coverage.
 *  I think this could extend to support RHI or vertical
 *  slicing DataTypes as well...vs the LatLonGrid 'CAPPI'
 *
 *  FIXME: Would be nice to combine everything somehow.
 *
 *  @author Robert Toomey
 */
class LatLonArea : public DataGrid {
public:

  /** Construct empty LatLonArea */
  LatLonArea() : myLatSpacing(0), myLonSpacing(0){ }

  // Cell information ability -------------------------------------------

  /** Get the latitude spacing in degrees between cells, if any. */
  AngleDegs
  getLatSpacing() const
  {
    return (myLatSpacing);
  }

  /** Get the longitude spacing in degrees between cells, if any. */
  AngleDegs
  getLonSpacing() const
  {
    return (myLonSpacing);
  }

  // Typically stored in dimension info, so we'll make these abstract

  /** Get the number of latitude cells */
  virtual size_t
  getNumLats() const = 0;

  /** Get the number of longitude cells */
  virtual size_t
  getNumLons() const = 0;

  // Layer information ability
  // typically height in meters, but subclasses might use for other things

  /** Get the number of layers */
  virtual size_t
  getNumLayers() const = 0;

  /** Get the layer value for given level. */
  virtual int
  getLayerValue(size_t l) = 0;

  /** Set the layer value for given level. */
  virtual void
  setLayerValue(size_t l, int v) = 0;

  /** Set the latitude/longitude spacing used */
  void
  setSpacing(AngleDegs lat_spacing, AngleDegs lon_spacing);

  /** Get a LLCoverageArea matching our grid */
  LLCoverageArea
  getLLCoverageArea();

  /** Get the top left location of a cell
   *  This is the point on the left top of cell (see X).
   *  X------
   *  |     |
   *  |     |
   *  |     |
   *  -------
   */
  LLH
  getTopLeftLocationAt(size_t i, size_t j);

  /** Get the center location of a cell
   *  This is the center point of the cell (see O).
   *  -------
   *  |     |
   *  |  O  |
   *  |     |
   *  -------
   */
  LLH
  getCenterLocationAt(size_t i, size_t j);

  /** Return the location considered the 'center' location of the datatype */
  virtual LLH
  getCenterLocation() const override;

  /** Update global attribute list */
  virtual void
  updateGlobalAttributes(const std::string& encoded_type) override;

  /** Sync any internal stuff to data from current attribute list,
   * return false on fail. */
  virtual bool
  initFromGlobalAttributes() override;

  /** Friend the output operator */
  friend std::ostream&
  operator << (std::ostream&, const rapio::LatLonArea&);

protected:

  /** Deep copy our fields to a new subclass */
  void
  deep_copy(std::shared_ptr<LatLonArea> n);

  /** Latitude spacing of cells in degrees */
  AngleDegs myLatSpacing;

  /** Longitude spacing of cells in degrees */
  AngleDegs myLonSpacing;
};

/** Output a LonLonHeightGrid info */
std::ostream&
operator << (std::ostream&,
  const rapio::LatLonArea&);
}
