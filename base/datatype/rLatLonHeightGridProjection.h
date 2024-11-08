#pragma once

#include <rDataProjection.h>
#include <rArray.h>

#include <string>

namespace rapio
{
class LatLonHeightGrid;

/** Temp just make a LatLonGridHeight to projection in 2D so we can make
 * quick tiles.
 * FIXME: Don't store array pointed in the projection class, just
 * return the grid coordinates for callers...
 * in otherwords, just make it convert from say lat lon to x, y */
class LatLonHeightGridProjection : public DataProjection
{
public:

  /** Create a lat lon grid projection */
  LatLonHeightGridProjection(const std::string& layer, LatLonHeightGrid * owner);

  /** Get a value at a lat lon for a given layer name (SpaceTime datatype) */
  virtual double
  getValueAtLL(double latDegs, double lonDegs) override;

  /** Calculate Lat Lon coverage marching grid from spatial center */
  virtual bool
  LLCoverageCenterDegree(const float degreeOut, const size_t numRows, const size_t numCols,
    float& topDegs, float& leftDegs, float& deltaLatDegs, float& deltaLonDegs) override;

  /** Calculate a 'full' which hits all cells of the lat lon grid */
  virtual bool
  LLCoverageFull(size_t& numRows, size_t& numCols,
    float& topDegs, float& leftDegs, float& deltaLatDegs, float& deltaLonDegs) override;

  /** Calculate a coverage based on the tiles */
  virtual bool
  LLCoverageTile(const size_t zoomLevel, const size_t& numRows, const size_t& numCols,
    const float centerLatDegs, const float centerLonDegs,
    float& topDegs, float& leftDegs, float& deltaLatDegs, float& deltaLonDegs) override;

protected:

  /** Cache current set 2D layer */
  ArrayFloat3DPtr my3DLayer;

  /** Cache Latitude degrees */
  float myLatNWDegs;

  /** Cache Longitude degrees */
  float myLonNWDegs;

  /** Cache Latitude spacing */
  float myLatSpacing;

  /** Cache Longitude spacing */
  float myLonSpacing;

  /** Cache number of lats */
  size_t myNumLats;

  /** Cache number of lons */
  size_t myNumLons;
};
}
