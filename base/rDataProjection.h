#pragma once

#include <rUtility.h>
#include <rConstants.h>
#include <rArray.h>
#include <rProject.h>

#include <string>
#include <map>

namespace rapio
{
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
 *
 * A temporary wrapper/view to a DataType
 * that allows accessing it in a geometric manner.
 *
 * Note:  This class is designed to exist only when the
 * underlying DataType class exists.  For speed we just use
 * pointers, so if for example, you let the underlying RadialSet
 * shared_ptr be deleted than the projection created is also invalid.
 * We 'could' maybe use weak_ptr at some point.
 *
 * @author Robert Toomey */
class DataProjection : public Utility
{
public:

  /** Create a data projection with default layer name */
  DataProjection() : myLayerName(Constants::PrimaryDataName){ }

  /** Create a data projection for a given layer name */
  DataProjection(const std::string& layer) : myLayerName(layer){ }

  /** Get a value at a lat lon for a given layer name (SpaceTime datatype) */
  virtual double
  getValueAtLL(double latDegs, double lonDegs)
  {
    return Constants::DataUnavailable;
  }

  /** Calculate marching box for generating square images */
  static std::shared_ptr<ProjLibProject>
  getBBOX(std::map<std::string, std::string>& keys,
    size_t& rows, size_t& cols, double& left, double& bottom, double& right, double& top);

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

protected:

  /** On demand projector from webmerc to lat lon */
  static std::shared_ptr<ProjLibProject> theWebMercToLatLon;

  /** Layer name we are linked to */
  std::string myLayerName;
};
}
