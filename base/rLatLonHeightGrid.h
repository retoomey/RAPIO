#pragma once

#include <rDataGrid.h>
#include <rLLH.h>
#include <rTime.h>
#include <rLatLonGrid.h>
#include <vector>

namespace rapio {
/** Store an area of data on a uniform 3-D grid of latitude and
 *  longitude at a constant height above the surface of the earth,
 *  where there are N levels of height.
 *
 *  First implementation we will do this as a stack of LatLonGrids,
 *  though in certain cases having a direct 3D array might be better.
 *  Each LatLonGrid will have additional meta properties stores by
 *  the height grid.
 *
 *  Second implementation could use a Data3D and only allow 3D iteration.
 *  Interestingly RadialSet could for example also be a N collection of
 *  Radials which is how W2 did it.  So multiple internal representations
 *  might be something to do.
 *
 *  FIXME: I might enhance get DataType to allow parameters or tweaking as
 *  to the method of storage, this would allow faster iteration in
 *  certain situations
 *
 *  @author Robert Toomey
 */
class LatLonHeightGrid : public DataGrid {
public:
  // Humm how are we gonna do projection with this thing?
  // We could basically pick a 2D layer at moment for testing
  // FIXME:  We'll pass general params to the projection object
  // which will allow 2d, vslice, etc.
  friend LatLonHeightGridProjection;

  /** Construct uninitialized LatLonHeightGrid, usually for
   * factories.  You probably want the Create method */
  LatLonHeightGrid();

  /** Public API for users to create a single band LatLonHeightGrid quickly,
   * Note that data is uninitialized/random memory since most algorithms
   * you'll fill it in and it wastes time to double fill it. */
  static std::shared_ptr<LatLonHeightGrid>
  Create(
    const std::string& TypeName,
    const std::string& Units,
    const LLH        & northwest,
    const Time       & gridtime,
    float            lat_spacing,
    float            lon_spacing,
    size_t           num_lats,
    size_t           num_lons,
    size_t           num_levels);

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

  /** Get the number of layers */
  size_t
  getNumLayers()
  {
    return myLayerNumbers.size();
  }

  /** Return a LatLonGrid of the given layer index, which can then be filled in normally.
   * With the N 2D implementation this will lazy create/cache a LatLonGrid 2D.
   * If I do the 3D array I'll need a 'view' object */
  std::shared_ptr<LatLonGrid>
  getLatLonGrid(size_t layerNumber);

  /** Return the location considered the 'center' location of the datatype */
  virtual LLH
  getCenterLocation() override
  {
    const double lat = myLocation.getLatitudeDeg() - myLatSpacing * (getNumLats() / 2.0);
    const double lon = myLocation.getLongitudeDeg() + myLonSpacing * (getNumLons() / 2.0);

    // FIXME: What should this represent with a 3D object?
    return LLH(lat, lon, myLocation.getHeightKM());
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
    size_t           num_lons,
    size_t           num_layers);

protected:

  /** Latitude spacing of cells in degrees */
  float myLatSpacing;

  /** Longitude spacing of cells in degrees */
  float myLonSpacing;

  /** The number of latitude cells for all layers */
  size_t myNumLats;

  /** The number of latitude cells for all layers */
  size_t myNumLons;

  /** Vector of layer numbers.  Most likely heights */
  std::vector<int> myLayerNumbers;

  /** LatLonGrids are independent objects */
  std::vector<std::shared_ptr<LatLonGrid> > myLatLonGrids;
};
}
