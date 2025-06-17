#pragma once

#include "rWgribCallback.h"

#include <iostream>

namespace rapio {
/**
 * Callback that dumps the catalog using wgrib2.
 * Technically since we just print out the list
 * we don't need a callback here.
 *
 * Remember callbacks should directly use
 * std::cout not logging since we capture this
 * from wgribw
 *
 * @author Robert Toomey
 */
class CatalogCallback : public WgribCallback {
public:

  /** Initialize a Catalog callback */
  CatalogCallback(const URL& u) : WgribCallback(u)
  { }

  /** Execute the callback, calling wgrib2 */
  //  void
  //  execute() override;

  /** Initialize at the start of a grib2 catalog pass */
  void
  handleInitialize(int * decode, int * latlon) override;

  /** Finalize at the end of a grib2 catalog pass */
  void
  handleFinalize() override;

  // -----------------------------------------------
  // We don't handle dealing with data, but the C table
  // needs the functions anyway.

  /** Calculate a minimum LLCoverageArea based on grib2 extents */
  void
  handleSetLatLon(double * lat, double * lon, size_t nx, size_t ny) override;

  /** Get a LLCoverageArea wanted for grid interpolation. */
  void
  handleGetLLCoverageArea(double * nwLat, double * nwLon,
    double * seLat, double * seLon, double * dLat, double * dLon,
    int * nLat, int * nLon) override;
};
}
