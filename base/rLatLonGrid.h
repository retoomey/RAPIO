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

  /** Public API for users to create a single band LatLonGrid quickly */
  static std::shared_ptr<LatLonGrid>
  Create(
    const std::string& TypeName,
    const std::string& Units,
    const LLH        & northwest,
    const Time       & gridtime,
    float            lat_spacing,
    float            lon_spacing,
    size_t           num_lats,
    size_t           num_lons,
    float            value = Constants::MissingData);

  // ------------------------------------------------------
  // Getting the 'data' of the 2d array...
  // @see DataGrid for accessing the various raster layers

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

  /** Generated default string for subtype from the data */
  virtual std::string
  getGeneratedSubtype() const override;

  /** Projection for data type */
  virtual std::shared_ptr<DataProjection>
  getProjection(const std::string& layer) override;

  // ------------------------------------------------------------
  // Methods for factories, etc. to fill in data post creation
  // Normally you don't call these directly unless you are making
  // a factory

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
    float            value);

public:

  /** Update global attribute list */
  virtual void
  updateGlobalAttributes(const std::string& encoded_type) override;

  /** Sync any internal stuff to data from current attribute list,
   * return false on fail. */
  virtual bool
  initFromGlobalAttributes() override;

protected:

  /** Latitude spacing of cells in degrees */
  float myLatSpacing;

  /** Longitude spacing of cells in degrees */
  float myLonSpacing;
};
}
