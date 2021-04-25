#pragma once

#include <rUtility.h>
#include <rDataGrid.h>
#include <rLatLonGrid.h>

#include <string>
#include <memory>

#define ACCEPT_USE_OF_DEPRECATED_PROJ_API_H
#include <proj.h>
#include <proj_api.h>

namespace rapio {
/** Create a generic projection API wrapper
 *
 * @author Robert Toomey
 */
class Project : public Utility {
public:

  /** Initialize the projection system for this object */
  virtual bool
  initialize();

  /** Project from a lat lon to azimuth range based on earth surface */
  static void
  LatLonToAzRange(const float &cLat, const float &cLon,
    const float &tLat, const float &tLon, float &azDegs, float &rangeMeters);

  /** Create Lat Lon Grid marching information from a center and delta degree */
  static void
  createLatLonGrid(const float centerLatDegs, const float centerLonDegrees,
    const float degreeOut, const size_t numRows, const size_t numCols,
    float& topDegs, float& leftDegs, float& deltaLatDegs, float& deltaLonDegs);

  /** Get the X/Y kilometer coordinate origin for the raster grid center */
  virtual bool
  getXYCenter(double& centerXKm, double& centerYKm);

  /** Get Lat Lon from given X/Y kilometer coordinates */
  virtual bool
  xyToLatLon(double& x, double&y, double &lat, double&lon);

  /** Create a lookup grid that has an index for a WDSSII lat lon grid.
   *  I'm suspecting we can do this generically
   * The basic technique is find the X/Y coordinates of the projected center,
   * then kilometer 'march' out from the center
   * Projection is from these X/Y kilometer coordinates to Lat/Lon...
   * 1. F(X/Y) --> F(Lat,Lon)
   * Then using the input lat lon information, we find the index into it
   * 2. F(Lat, Lon) --> F(I,J)
   */
  virtual int
  createLookup(
    // Output information
    int   desired_rows, // rows of output projection
    int   desired_cols, // cols of output projection
    int   mCell,        // Kilometers per cell (sample rate)

    // Input lat lon grid information for projecting lat/lon to index
    // Should we pass the lat lon grid?
    float topleftlat,
    float topleftlon,
    float numlat,
    float numlon,
    float latres,
    float lonres);

  /** Project array to lat lon grid.  This isn't super generic */
  virtual void
  toLatLonGrid(std::shared_ptr<Array<float, 2> > in,
    std::shared_ptr<LatLonGrid>           out){ };
};

/* The only subclass for now, projects using the
 * Proj library.  We could have Project be a factory.
 *
 * @author Robert Toomey
 */
class ProjLibProject : public Project
{
public:

  /** Send needed information to the class for initialization */
  ProjLibProject(const std::string& src, const std::string& dst);

  ~ProjLibProject()
  {
    if (myP != nullptr) {
      proj_destroy(myP);
    }
  }

  /** Initialize the projection system */
  virtual bool
  initialize() override;

  /** Get the X/Y kilometer coordinate origin for the raster grid center */
  virtual bool
  getXYCenter(double& centerXKm, double& centerYKm) override;

  /** Get Lat Lon from given X/Y kilometer coordinates */
  virtual bool
  xyToLatLon(double& x, double&y, double &lat, double&lon) override;

  /** Project array to lat lon grid primary.  This isn't super generic. */
  virtual void
  toLatLonGrid(std::shared_ptr<Array<float, 2> > in,
    std::shared_ptr<LatLonGrid>           out) override;
private:

  /** Source projection string in Proj library language */
  std::string mySrc;

  /** Destination projection string in Proj library language */
  std::string myDst;

  /** Proj library object to handle source projection */
  projPJ pj_src;

  /** Proj library object to handle destination projection */
  projPJ pj_dst;

  /** Proj6 object */
  PJ * myP;
};
}
