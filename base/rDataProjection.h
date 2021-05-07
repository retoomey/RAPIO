#pragma once

#include <rUtility.h>
#include <rConstants.h>
#include <rArray.h>

#include <string>

namespace rapio
{
class RadialSet;
class RadialSetLookup;
class LatLonGrid;
class PTreeNode;

/** Simple storage object for getting projection info */
class LLCoverage : public Data
{
public:
  LLCoverage() : mode("full"), zoomlevel(0), rows(500), cols(500), degreeOut(10), topDegs(0),
    leftDegs(0), deltaLatDegs(0), deltaLonDegs(0), centerLatDegs(0), centerLonDegs(0){ }

  std::string mode;
  size_t zoomlevel;
  size_t rows;
  size_t cols;
  float degreeOut;
  float topDegs;
  float leftDegs;
  float deltaLatDegs;
  float deltaLonDegs;
  float centerLatDegs;
  float centerLonDegs;
};

std::ostream&
operator << (std::ostream&,
  const LLCoverage&);

/** Base class for projection of data.
 * Typically this object is a temporary wrapper to a DataType
 * that allows accessing it in a geometric manner.
 *
 * @author Robert Toomey */
class DataProjection : public Utility
{
public:

  /** Get a value at a lat lon for a given layer name (SpaceTime datatype) */
  virtual double
  getValueAtLL(double latDegs, double lonDegs, const std::string& layer = "primary")
  {
    return Constants::MissingData;
  }

  /** Calculate BBOX and BBOXSR */
  virtual bool
  getBBOX(std::map<std::string, std::string>& keys,
    size_t& rows, size_t& cols, std::string& bbox, std::string& bboxsr);

  /** Create projection based on standard fields passed in */
  virtual bool
  getLLCoverage(const PTreeNode& fields, LLCoverage& c);

  /** Calculate Lat Lon coverage marching grid from spatial center */
  virtual bool
  LLCoverageCenterDegree(const float degreeOut, const size_t numRows, const size_t numCols,
    float& topDegs, float& leftDegs, float& deltaLatDegs, float& deltaLonDegs){ return false; }

  /** Calculate a 'full' coverage based on the data type */
  virtual bool
  LLCoverageFull(size_t& numRows, size_t& numCols,
    float& topDegs, float& leftDegs, float& deltaLatDegs, float& deltaLonDegs){ return false; }

  /** Calculate a coverage based on the tiles */
  virtual bool
  LLCoverageTile(const size_t zoomLevel, const size_t& numRows, const size_t& numCols,
    const float centerLatDegs, const float centerLonDegs,
    float& topDegs, float& leftDegs, float& deltaLatDegs, float& deltaLonDegs){ return false; }
};

class LatLonGridProjection : public DataProjection
{
public:

  /** Create a lat lon grid projection */
  LatLonGridProjection(const std::string& layer, LatLonGrid * owner);

  /** Get a value at a lat lon for a given layer name (SpaceTime datatype) */
  virtual double
  getValueAtLL(double latDegs, double lonDegs, const std::string& layer = "primary") override;

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

  /** Cache current set 2D layer */
  std::shared_ptr<Array<float, 2> > my2DLayer;

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

class RadialSetProjection : public DataProjection
{
public:

  /** Create a radial set projection */
  RadialSetProjection(const std::string& layer, RadialSet * owner);

  /** Get a value at a lat lon for a given layer name (SpaceTime datatype) */
  virtual double
  getValueAtLL(double latDegs, double lonDegs, const std::string& layer = "primary") override;

  /** Calculate Lat Lon coverage marching grid from spatial center */
  virtual bool
  LLCoverageCenterDegree(const float degreeOut, const size_t numRows, const size_t numCols,
    float& topDegs, float& leftDegs, float& deltaLatDegs, float& deltaLonDegs) override;

  virtual bool
  LLCoverageFull(size_t& numRows, size_t& numCols,
    float& topDegs, float& leftDegs, float& deltaLatDegs, float& deltaLonDegs) override;

  /** Cache center degrees */
  float myCenterLatDegs;

  /** Cache center degrees */
  float myCenterLonDegs;

  /** Cache current set 2D layer */
  std::shared_ptr<Array<float, 2> > my2DLayer;

  /** Reference to Radial set lookup */
  std::shared_ptr<RadialSetLookup> myLookup;
};
}
