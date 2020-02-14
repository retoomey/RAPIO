#pragma once

#include <rDataGrid.h>
#include <rLLH.h>
#include <rTime.h>

#include <vector>

namespace rapio {
/** Store an area of double data on a uniform 2-D grid of latitude and
 *  longitude at a constant height above the surface of the earth.
 *
 *  @author Robert Toomey
 */
class LatLonGrid : public DataGrid {
public:

  /** Construct uninitialized LatLonGrid, usually for
   * factories */
  LatLonGrid();

  /** Create a lat lon grid object */
  LatLonGrid(
    const LLH&,
    const Time&,
    float  lat_spacing,
    float  lon_spacing,
    size_t rows = 0,
    size_t cols = 0,
    float  value = 0.0f);

  /** Post creation initialization (for testing automation)
   * Resize can change data size. */
  void
  init(
    const LLH   & location,
    const Time  & time,
    const float lat_spacing,
    const float lon_spacing);

  // ------------------------------------------------------
  // Getting the 'metadata' on the 2d array
  virtual LLH
  getLocation() const override
  {
    return (myLocation);
  }

  virtual Time
  getTime() const override
  {
    return (myTime);
  }

  float
  getLatSpacing() const
  {
    return (myLatSpacing);
  }

  float
  getLonSpacing() const
  {
    return (myLonSpacing);
  }

  // ------------------------------------------------------
  // Getting the 'data' of the 2d array...

  size_t
  getNumLats()
  {
    return myDims[0].size();
  }

  size_t
  getNumLons()
  {
    return myDims[1].size();
  }

  virtual std::string
  getGeneratedSubtype() const override;

  /** Sync any internal stuff to data from current attribute list,
   * return false on fail. */
  virtual bool
  initFromGlobalAttributes() override;

  /** Update global attribute list for RadialSet */
  virtual void
  updateGlobalAttributes(const std::string& encoded_type) override;

protected:

  // This is projection information.  Basically HOW the 2D data
  // is in spacetime, which is separate from the 2D data.
  // I feel this could be designed cleaner to
  // avoid 'cartesiangrid' like stuff like in WDSS2

  /** Both the Time corresponding to this grid as a whole and the
   *  Location of the extreme northwest corner grid point.
   */
  LLH myLocation;
  Time myTime;
  float myLatSpacing;
  float myLonSpacing;
};
}
