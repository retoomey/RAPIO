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

  /** resizes to a ROW x COL grid and sets each cell to `value' */
  void
  resize(size_t rows, size_t cols, const float fill = 0)
  {
    //  myNumLats = rows;
    //  myNumLons = cols;
    // myData.resize(rows * cols, fill);
    // FIXME: Unavailable default?
    myFloat2DData[0].resize(cols, rows, fill);
  }

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
    return myFloat2DData[0].getY();
  }

  size_t
  getNumLons()
  {
    return myFloat2DData[0].getX();
  }

  /** Sparse2D template wants this method (read) */
  void
  set(size_t i, size_t j, const float& v) override
  {
    myFloat2DData[0].set(i, j, v);
  }

  void
  fill(const float& value) override
  {
    std::fill(myFloat2DData[0].begin(), myFloat2DData[0].end(), value);
  }

  virtual std::string
  getGeneratedSubtype() const override;

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
