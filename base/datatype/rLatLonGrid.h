#pragma once

#include <rLatLonArea.h>
#include <rLLH.h>
#include <rTime.h>
#include <rLLCoverageArea.h>

namespace rapio {
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

  /** Public API for users to create a single band LatLonGrid quickly */
  static std::shared_ptr<LatLonGrid>
  Create(
    const std::string    & TypeName,
    const std::string    & Units,
    const Time           & gridtime,
    const LLCoverageArea & grid);

  /** Public API for users to clone a LatLonGrid */
  std::shared_ptr<LatLonGrid>
  Clone();

  /** Get the number of latitude cells */
  virtual size_t
  getNumLats() const override
  {
    return myDims.size() > 0 ? myDims[0].size() : 0;
  }

  /** Get the number of longitude cells */
  virtual size_t
  getNumLons() const override
  {
    return myDims.size() > 1 ? myDims[1].size() : 0;
  }

  /** Get number of layers.  For a LatLonGrid there is only one */
  virtual size_t
  getNumLayers() const override
  {
    return 1;
  }

  /** Get the layer value for given level. */
  virtual int
  getLayerValue(size_t l) override
  {
    // Just use the height in meters
    return myLocation.getHeightKM() * 1000.0;
  }

  /** Set the layer value for given level. */
  virtual void
  setLayerValue(size_t l, int v) override
  {
    if (l == 0) {
      float heightKM = v / 1000.0;
      myLocation.setHeightKM(heightKM);
    }
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
