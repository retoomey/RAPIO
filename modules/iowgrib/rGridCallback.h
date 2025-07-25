#pragma once

#include "rWgribCallback.h"
#include "rLLCoverageArea.h"
#include "rURL.h"

#include "rLatLonGrid.h" // temp

#include <iostream>

namespace rapio {
/**
 * Callback that processes grid data.
 * Basic projection using wgrib2 and return of
 * grids 2D/3D.  We should be able to get
 * arrays of data and/or LatLonGrids.
 *
 * Remember callbacks should directly use
 * std::cout not logging since we capture this
 * from wgribw
 *
 * @author Robert Toomey
 */
class GridCallback : public WgribCallback {
public:

  /** Initialize a Catalog callback */
  GridCallback(const URL& u, const std::string& match);

  /** Initialize at the start of a grib2 catalog pass */
  void
  handleInitialize(int * decode, int * latlon) override;

  /** Finalize at the end of a grib2 catalog pass */
  void
  handleFinalize() override;

  /** Return action type for the c module */
  ActionType
  handleGetActionType() override { return ACTION_LATLONGRID; }

  /** Calculate a minimum LLCoverageArea based on grib2 extents */
  void
  handleSetLatLon(double * lat, double * lon, size_t nx, size_t ny) override;

  /** Get a LLCoverageArea wanted for grid interpolation. */
  void
  handleGetLLCoverageArea(double * nwLat, double * nwLon,
    double * seLat, double * seLon, double * dLat, double * dLon,
    int * nLat, int * nLon) override;

  /** Called with raw data, unprojected */
  virtual void
  handleSetDataArray(float * data, int nlats, int nlons, unsigned int * index) override;

  /** Our C++ get coverage area.  Used for wgrib2 and output */
  LLCoverageArea
  getLLCoverageArea()
  {
    return myLLCoverageArea;
  }

  /** Pull the current LatLonGrid, taking ownership */
  static std::shared_ptr<LatLonGrid>
  pullLatLonGrid()
  {
    auto temp = myTempLatLonGrid;

    myTempLatLonGrid = nullptr;
    return temp;
  }

protected:

  /** Temp storage of returned LatLonGrid */
  static std::shared_ptr<LatLonGrid> myTempLatLonGrid;

  /** Current coverage area wanted for output */
  LLCoverageArea myLLCoverageArea;
};
}
