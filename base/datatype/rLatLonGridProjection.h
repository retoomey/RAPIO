#pragma once

#include <rDataProjection.h>
#include <rArray.h>
#include <rArraySampler.h>

#include <string>

namespace rapio
{
class LatLonGrid;

/**
 * @brief Fast-path coordinate mapper for LatLonGrid to LatLonGrid resampling.
 *
 * Architecturally, there is an overlap here between the ImagePipeline (array math)
 * and DataProjection (geospatial physics). Strictly speaking, mapping between grids
 * should use full geographic projections (Dest_XY -> Lat/Lon -> Source_UV).
 * However, executing full geodetic math for every pixel is computationally expensive.
 *
 * Because this mapper assumes both the source and destination share the same
 * geometric class (LatLonGrid), it safely bypasses the heavy geographic math and
 * uses highly optimized O(1) linear scaling.
 */
struct LatLonGridMapper : public ArrayMapper {
  double myInNWLat, myInNWLon, myInLatSpacing, myInLonSpacing;
  double myOutStartLat, myOutStartLon, myOutLatSpacing, myOutLonSpacing;

  LatLonGridMapper(const LatLonGrid& source, const LatLonGrid& dest);

  void
  map2D_Y(size_t destI_start, size_t count, float * outU) const override
  {
    for (size_t i = 0; i < count; ++i) {
      double atLat = myOutStartLat - ((destI_start + i) * myOutLatSpacing);
      outU[i] = static_cast<float>((myInNWLat - atLat) / myInLatSpacing);
    }
  }

  void
  map2D_X(size_t destJ_start, size_t count, float * outV) const override
  {
    for (size_t j = 0; j < count; ++j) {
      double atLon = myOutStartLon + ((destJ_start + j) * myOutLonSpacing);
      outV[j] = static_cast<float>((atLon - myInNWLon) / myInLonSpacing);
    }
  }
};

class LatLonGridProjection : public DataProjection
{
public:

  /** Create a lat lon grid projection */
  LatLonGridProjection(const std::string& layer, LatLonGrid * owner);

  /** Get a value at a lat lon for a given layer name (SpaceTime datatype) */
  virtual double
  getValueAtLL(double latDegs, double lonDegs) override;

  /** Batch get values for Lat Lon */
  virtual void
  getValuesAtLL(const double * lats, const double * lons,
    float * out, size_t count) override;

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
  ArrayFloat2DPtr my2DLayer;

  /** Cache Latitude degrees */
  float myLatNWDegs;

  /** Cache Longitude degrees */
  float myLonNWDegs;

  /** Cache Latitude spacing */
  float myLatSpacing;

  /** Cache the inverse of Latitude spacing */
  float myInvLatSpacing;

  /** Cache Longitude spacing */
  float myLonSpacing;

  /** Cache the inverse of Longitude spacing */
  float myInvLonSpacing;

  /** Cache the longitude width */
  double myLonWidth;

  /** Cache number of lats */
  size_t myNumLats;

  /** Cache number of lons */
  size_t myNumLons;
};
}
