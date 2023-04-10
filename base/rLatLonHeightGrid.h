#pragma once

#include <rLatLonGrid.h>

namespace rapio {
/** Store an area of data on a uniform 3-D grid of latitude and
 *  longitude at a constant height above the surface of the earth,
 *  where there are N levels of height.
 *
 *  First implementation using 3D array.
 *
 *  FIXME: I might enhance get DataType to allow parameters or tweaking as
 *  to the method of storage, this would allow faster iteration in
 *  certain situations
 *
 *  We 'could' add layer 'n' ability to the LatLonGrid, but to match WDSS2
 *  which has distinct classes, we add the 3D onto our 2D class here.
 *
 *  @author Robert Toomey
 */
class LatLonHeightGrid : public LatLonGrid {
public:
  // Humm how are we gonna do projection with this thing?
  // We could basically pick a 2D layer at moment for testing
  // FIXME:  We'll pass general params to the projection object
  // which will allow 2d, vslice, etc.
  friend LatLonHeightGridProjection;

  /** Construct uninitialized LatLonHeightGrid, usually for
   * factories.  You probably want the Create method */
  LatLonHeightGrid();

  /** Create a LatLonHeightGrid */
  LatLonHeightGrid(
    const std::string& TypeName,
    const std::string& Units,
    const LLH        & northwest,
    const Time       & gridtime,
    float            lat_spacing,
    float            lon_spacing,
    size_t           num_lats,
    size_t           num_lons,
    size_t           num_levels);

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

  /** Generated default string for subtype from the data */
  virtual std::string
  getGeneratedSubtype() const override;

  /** Projection for data type */
  virtual std::shared_ptr<DataProjection>
  getProjection(const std::string& layer) override;
};
}
