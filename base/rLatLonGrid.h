#pragma once

#include <rDataType.h>
#include <rLLH.h>
#include <rTime.h>

#include <vector>

namespace rapio {
/** Store an area of double data on a uniform 2-D grid of latitude and
 *  longitude at a constant height above the surface of the earth.
 */
class LatLonGrid : public DataType {
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
    myNumLats = rows;
    myNumLons = cols;
    myData.resize(rows * cols, fill);
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

  /** Return pointer to start of data for read/write */
  float *
  getDataVector()
  {
    return (&myData[0]);
  }

  // size_d()
  size_t
  getNumLats()
  {
    return (myNumLats);
  }

  // size_d(0);
  size_t
  getNumLons()
  {
    return (myNumLons);
  }

  /** Index for grid */
  inline size_t
  index(size_t i, size_t j)
  {
    // i = row, j = col.
    return ((i * myNumLons) + j);
  }

  /** Sparse2D template wants this method (read) */
  void
  set(size_t i, size_t j, const float& v) override
  {
    myData[index(i, j)] = v;
  }

  /** Get value at a i, j location, need at some point */

  // float get(size_t i, size_t j) {
  //   return myData[index(i, j)]; // copying
  // }

  void
  fill(const float& value) override
  {
    std::fill(myData.begin(), myData.end(), value);

    // myData.resize(myNumLats*myNumLons, value);
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

  // This is data information
  size_t myNumLats; // Also could be rows/cols or dimensions
  size_t myNumLons;

  // This is the data array.  It's stored in netcdf grid order for
  // convenience for reading/writing
  // FIXME: Should we allocate our own memory?  It would avoid C++
  // blocking extra space since in general we _know_ the exact size
  // we want
  std::vector<float> myData;
};
}
