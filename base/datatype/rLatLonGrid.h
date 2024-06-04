#pragma once

#include <rDataGrid.h>
#include <rLLH.h>
#include <rTime.h>

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
  getNumLats() = 0;

  /** Get the number of longitude cells */
  virtual size_t
  getNumLons() = 0;

  /** Set the latitude/longitude spacing used */
  void
  setSpacing(AngleDegs lat_spacing, AngleDegs lon_spacing);

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
  getCenterLocation() override;

  // Layer information ability -------------------------------------------

  /** Get the number of layers */
  virtual size_t
  getNumLayers()
  {
    return myLayerNumbers.size();
  }

  /** Return reference to layer numbers, which then can be used for searching or changing */
  std::vector<int>&
  getLayerValues()
  {
    return myLayerNumbers;
  }

  /** Set the layer value for given level.  This is the height currently */
  void
  setLayerValue(size_t l, int v)
  {
    myLayerNumbers[l] = v;
  }

  /** Get the layer value for given level.  This is the height currently */
  int
  getLayerValue(size_t l)
  {
    return myLayerNumbers[l];
  }

  /** Update global attribute list */
  virtual void
  updateGlobalAttributes(const std::string& encoded_type) override;

  /** Sync any internal stuff to data from current attribute list,
   * return false on fail. */
  virtual bool
  initFromGlobalAttributes() override;

protected:

  /** Deep copy our fields to a new subclass */
  void
  deep_copy(std::shared_ptr<LatLonArea> n);

  /** Latitude spacing of cells in degrees */
  AngleDegs myLatSpacing;

  /** Longitude spacing of cells in degrees */
  AngleDegs myLonSpacing;

  /** Vector of layer numbers.  Most likely heights. */
  std::vector<int> myLayerNumbers;
};

/** Store an area of data on a uniform 2-D grid of latitude and
 *  longitude at a constant height above the surface of the earth.
 *
 *  @author Robert Toomey
 */
class LatLonGrid : public LatLonArea {
public:
  friend LatLonGridProjection;

  /** Construct uninitialized LatLonGrid, usually for
   * factories.  You probably want the Create method */
  LatLonGrid();

  /** Public API for users to create a single band LatLonGrid quickly,
   * Note that data is uninitialized/random memory since most algorithms
   * you'll fill it in and it wastes time to double fill it. */
  static std::shared_ptr<LatLonGrid>
  Create(
    const std::string& TypeName,
    const std::string& Units,
    const LLH        & northwest,
    const Time       & gridtime,
    float            lat_spacing,
    float            lon_spacing,
    size_t           num_lats,
    size_t           num_lons);

  /** Public API for users to clone a LatLonGrid */
  std::shared_ptr<LatLonGrid>
  Clone();

  /** Get the number of latitude cells */
  virtual size_t
  getNumLats() override
  {
    return myDims.size() > 0 ? myDims[0].size() : 0;
  }

  /** Get the number of longitude cells */
  virtual size_t
  getNumLons() override
  {
    return myDims.size() > 1 ? myDims[1].size() : 0;
  }

  /** Generated default string for subtype from the data */
  virtual std::string
  getGeneratedSubtype() const override;

  /** Projection for data type */
  virtual std::shared_ptr<DataProjection>
  getProjection(const std::string& layer) override;

  /** Handle post read by sparse uncompression if wanted */
  virtual void
  postRead(std::map<std::string, std::string>& keys) override;

  /** Make ourselves MRMS sparse iff we're non-sparse.  This keeps
   * any DataGrid writers like netcdf generic not knowing about our
   * special sparse formats. */
  virtual void
  preWrite(std::map<std::string, std::string>& keys) override;

  /** Make ourselves MRMS non-sparse iff we're sparse */
  virtual void
  postWrite(std::map<std::string, std::string>& keys) override;

protected:
  /** Initialize a LatLonGrid post create */
  bool
  init(
    const std::string & TypeName,
    const std::string & Units,
    const LLH         & northwest,
    const Time        & gridtime,
    float             lat_spacing,
    float             lon_spacing,
    size_t            num_lats,
    size_t            num_lons
  );

  /** Deep copy our fields to a new subclass */
  void
  deep_copy(std::shared_ptr<LatLonGrid> n);
};
}
