#pragma once

#include <rLatLonGrid.h>
#include <rLLCoverageArea.h>

namespace rapio {
/** Store an area of data on a uniform 3-D grid of latitude and
 *  longitude at a constant height above the surface of the earth,
 *  where there are N levels of height.
 *
 *  Implementation using 3D array.
 *
 *  @author Robert Toomey
 */
class LatLonHeightGrid : public LatLonArea {
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

  /** Public API for users to create a single band LatLonHeightGrid quickly */
  static std::shared_ptr<LatLonHeightGrid>
  Create(
    const std::string    & TypeName,
    const std::string    & Units,
    const Time           & gridtime,
    const LLCoverageArea & grid);

  /** Public API for users to clone a LatLonHeightGrid */
  std::shared_ptr<LatLonHeightGrid>
  Clone();

  /** Generated default string for subtype from the data */
  virtual std::string
  getGeneratedSubtype() const override;

  /** Projection for data type */
  virtual std::shared_ptr<DataProjection>
  getProjection(const std::string& layer) override;

  // MRMS switches the dimension order of 2D (x,y) to 3D (z,x,y) which
  // means we have the change how we get these.

  /** Get the number of latitude cells */
  virtual size_t
  getNumLats() const override
  {
    return myDims.size() > 1 ? myDims[1].size() : 0;
  }

  /** Get the number of longitude cells */
  virtual size_t
  getNumLons() const override
  {
    return myDims.size() > 2 ? myDims[2].size() : 0;
  }

  // Layer information ability
  virtual size_t
  getNumLayers() const override
  {
    // Subclass such as LLHGribN2D doesn't have myDims
    // set the same, so use the height array size
    // Other option is override it there
    // return myDims.size() > 0 ? myDims[0].size() : 0;
    auto array = getFloat1D("Height");

    return array ? array->getX() : 0;
  }

  /** Get the layer value for given level. */
  virtual int
  getLayerValue(size_t l) override
  {
    auto array = getFloat1D("Height");

    if (array) {
      auto& r = array->ref();
      return r[l];
    }
    return 0;
  }

  /** Set the layer value for given level. */
  virtual void
  setLayerValue(size_t l, int v) override
  {
    auto array = getFloat1D("Height");

    if (array) {
      auto& r = array->ref();
      r[l] = v;
    }
  }

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

  /** Initialize a LatLonHeightGrid */
  bool
  init(
    const std::string& TypeName,
    const std::string& Units,
    const LLH        & northwest,
    const Time       & gridtime,
    float            lat_spacing,
    float            lon_spacing,
    size_t           num_lats,
    size_t           num_lons,
    size_t           num_levels);

  /** Deep copy our fields to a new subclass */
  void
  deep_copy(std::shared_ptr<LatLonHeightGrid> n);
};
}
