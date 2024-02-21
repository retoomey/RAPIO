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

  /** Set the latitude/longitude spacing used */
  void
  setSpacing(float lat_spacing, float lon_spacing);

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
  virtual size_t
  getNumLats()
  {
    return myDims.size() > 0 ? myDims[0].size() : 0;
  }

  /** Get the number of longitude cells */
  virtual size_t
  getNumLons()
  {
    return myDims.size() > 1 ? myDims[1].size() : 0;
  }

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

  /** Return the location considered the 'center' location of the datatype */
  virtual LLH
  getCenterLocation() override;

  /** Get the top left location of a cell in the LatLonGrid
   *  This is the point on the left top of grid (see X).
   *  X------
   *  |     |
   *  |     |
   *  |     |
   *  -------
   */
  LLH
  getTopLeftLocationAt(size_t i, size_t j);

  /** Get the center location of a cell in the LatLonGrid
   *  This is the center point of the grid (see O).
   *  -------
   *  |     |
   *  |  O  |
   *  |     |
   *  -------
   */
  LLH
  getCenterLocationAt(size_t i, size_t j);

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

protected:

  /** Latitude spacing of cells in degrees */
  float myLatSpacing;

  /** Longitude spacing of cells in degrees */
  float myLonSpacing;

  /** Vector of layer numbers.  Most likely heights. */
  std::vector<int> myLayerNumbers;
};
}
