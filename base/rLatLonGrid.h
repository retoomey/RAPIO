#pragma once

#include <rDataGrid.h>
#include <rLLH.h>
#include <rTime.h>

namespace rapio {
/** Store an area of data on a uniform 2-D grid of latitude and
 *  longitude at a constant height above the surface of the earth.
 *
 *  @author Robert Toomey
 */
class LatLonGrid : public DataGrid {
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

  /** Get the latitude spacing in degrees between cells */
  float
  getLatSpacing() const
  {
    return (myLatSpacing);
  }

  /** Get the longitude spacing in degrees between cells */
  float
  getLonSpacing() const
  {
    return (myLonSpacing);
  }

  /** Get the number of latitude cells */
  size_t
  getNumLats()
  {
    return myDims.size() > 0 ? myDims[0].size() : 0;
  }

  /** Get the number of longitude cells */
  size_t
  getNumLons()
  {
    return myDims.size() > 1 ? myDims[1].size() : 0;
  }

  // Layer interface.  A LatLonGrid only has one layer at its height, while
  // LatLonHeightGrids have multiple
  // FIXME: Could be virtual layer if other classes are made.  I can see
  // doing something that stores non-height layers

  /** Get the number of layers */
  size_t
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

  /** Return the location considered the 'center' location of the datatype */
  virtual LLH
  getCenterLocation() override
  {
    // This simple one liner doesn't work, because the middle can be on a cell wall and
    // not in the center of the cell.  Imagine 2 cells..the true middle is the line
    // between them.  For three cells it is the middle of cell 1.
    // return(getCenterLocationAt(getNumLats()/2, getNumLons()/2);
    //
    // However, this does:
    const double latHalfWidth = (myLatSpacing * getNumLats()) / 2.0;
    const double lonHalfWidth = (myLonSpacing * getNumLons()) / 2.0;
    const double latDegs      = myLocation.getLatitudeDeg() - latHalfWidth;
    const double lonDegs      = myLocation.getLongitudeDeg() + lonHalfWidth;

    return LLH(latDegs, lonDegs, myLocation.getHeightKM());
  }

  /** Get the top left location of a cell in the LatLonGrid
   *  This is the point on the left top of grid (see X).
   *  X------
   *  |     |
   *  |     |
   *  |     |
   *  -------
   */
  LLH
  getTopLeftLocationAt(size_t i, size_t j)
  {
    if (i == j == 0) { return myLocation; }
    const double latDegs = myLocation.getLatitudeDeg() - (myLatSpacing * i);
    const double lonDegs = myLocation.getLongitudeDeg() + (myLonSpacing * j);

    return LLH(latDegs, lonDegs, myLocation.getHeightKM());
  }

  /** Get the center location of a cell in the LatLonGrid
   *  This is the center point of the grid (see O).
   *  -------
   *  |     |
   *  |  O  |
   *  |     |
   *  -------
   */
  LLH
  getCenterLocationAt(size_t i, size_t j)
  {
    const double latDegs = myLocation.getLatitudeDeg() - (myLatSpacing * (i + 0.5));
    const double lonDegs = myLocation.getLongitudeDeg() + (myLonSpacing * (j + 0.5));

    return LLH(latDegs, lonDegs, myLocation.getHeightKM());
  }

  /** Generated default string for subtype from the data */
  virtual std::string
  getGeneratedSubtype() const override;

  /** Projection for data type */
  virtual std::shared_ptr<DataProjection>
  getProjection(const std::string& layer) override;

  /** Update global attribute list */
  virtual void
  updateGlobalAttributes(const std::string& encoded_type) override;

  /** Sync any internal stuff to data from current attribute list,
   * return false on fail. */
  virtual bool
  initFromGlobalAttributes() override;

private:

  /** Post creation initialization of fields
   * Resize can change data size. */
  void
  init(
    const std::string& TypeName,
    const std::string& Units,
    const LLH        & location,
    const Time       & time,
    const float      lat_spacing,
    const float      lon_spacing,
    size_t           num_lats,
    size_t           num_lons);

protected:

  /** Latitude spacing of cells in degrees */
  float myLatSpacing;

  /** Longitude spacing of cells in degrees */
  float myLonSpacing;

  /** Vector of layer numbers.  Most likely heights. */
  std::vector<int> myLayerNumbers;
};
}
